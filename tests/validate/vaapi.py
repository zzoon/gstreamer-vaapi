# -*- Mode: Python -*- vi:si:et:sw=4:sts=4:ts=4:syntax=python
#
# Copyright (c) 2017,Hyunjun Ko <zzoon@igalia.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, write to the
# Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
# Boston, MA 02110-1301, USA.

"""
The Gstreamer VA-API GstValidate default testsuite
"""
import os
import signal
import io
import re

from launcher.apps.gstvalidate import GstValidatePipelineTestsGenerator, \
    GstValidateLaunchTest, GstValidateTestManager

from launcher.utils import Result, DEFAULT_TIMEOUT, Colors

VALGRIND_ERROR_CODE = 20
COREDUMP_SIGNALS = [-getattr(signal, s) for s in [
    'SIGQUIT', 'SIGILL', 'SIGABRT', 'SIGFPE', 'SIGSEGV', 'SIGBUS', 'SIGSYS',
    'SIGTRAP', 'SIGXCPU', 'SIGXFSZ', 'SIGIOT'] if hasattr(signal, s)] + [139]

TEST_MANAGER = "vaapi"

class GstValidateVaapiTest(GstValidateLaunchTest):

    def __init__(self, classname, options, reporter, pipeline_desc, filename, checksum_file,
                 timeout=DEFAULT_TIMEOUT, scenario=None,
                 media_descriptor=None, duration=0, hard_timeout=None,
                 extra_env_variables=None, expected_failures=None):
        self.basename = os.path.basename(filename)
        self.checksum_file = checksum_file

        GstValidateLaunchTest.__init__(self, classname, options, reporter, pipeline_desc, scenario=scenario)

    def _check_checksum(self):
        self.otc_md5 = None
        self.vpg_md5 = None
        if self.logfile:
            f = open(self.logfile, 'r+')

            for line in f.readlines():
                asc = 1
                line = line.replace('\n', '')
                if ' ' in line:
                    continue
                elif len (line) == 0 or len (line) == 1:
                    continue
                else:
                    for c in line:
                        if (ord(c) < 48 or ord(c) > 57) and (ord(c) < 64 or ord(c) > 122):
                            asc = 0
                            break

                        if ord(c) == 10:
                            asc = 0
                            break

                    if len(line) != 32:
                        asc = 0

                    if asc == 1:
                        self.otc_md5 = line
                        break
            f.close()

        if self.otc_md5 == None:
            return False

        self.info("Checksum : %s %d", self.otc_md5, len (self.otc_md5))

        f = open(self.checksum_file, 'r+')

        for line in f.readlines():
            line = line.replace('\n', '')
            if self.basename in line:
                self.vpg_md5 = line.split()[1]
                break
        f.close()

        if self.vpg_md5 == None:
            return False

        if self.otc_md5 != self.vpg_md5:
            return False

        return True

    def check_results(self):
        if self.result in [Result.FAILED, self.result is Result.PASSED]:
            return

        for report in self.reports:
            if report.get('issue-id') == 'runtime::missing-plugin':
                self.set_result(Result.SKIPPED, "%s\n%s" % (report['summary'],
                                                         report['details']))
                return

        self.debug("%s returncode: %s", self, self.process.returncode)

        criticals, not_found_expected_failures, expected_returncode = self.check_reported_issues()

        expected_timeout = None
        for i, f in enumerate(not_found_expected_failures):
            if len(f) == 1 and f.get("returncode"):
                returncode = f['returncode']
                if not isinstance(expected_returncode, list):
                    returncode = [expected_returncode]
                if 'sometimes' in f:
                    returncode.append(0)
            elif f.get("timeout"):
                expected_timeout = f

        not_found_expected_failures = [f for f in not_found_expected_failures
                                       if not f.get('returncode')]

        msg = ""
        result = Result.PASSED
        if self.result == Result.TIMEOUT:
            if expected_timeout:
                not_found_expected_failures.remove(expected_timeout)
                result, msg = self.check_expected_timeout(expected_timeout)
            else:
                return
        elif self.process.returncode in COREDUMP_SIGNALS:
            result = Result.FAILED
            msg = "Application segfaulted "
            self.add_stack_trace_to_logfile()
        elif self.process.returncode == VALGRIND_ERROR_CODE:
            msg = "Valgrind reported errors "
            result = Result.FAILED
        elif self.process.returncode not in expected_returncode:
            msg = "Application returned %s " % self.process.returncode
            if expected_returncode != [0]:
                msg += "(expected %s) " % expected_returncode
            result = Result.FAILED

        if criticals:
            msg += "(critical errors: [%s]) " % ', '.join(criticals)
            result = Result.FAILED

        if not_found_expected_failures:
            mandatory_failures = [f for f in not_found_expected_failures
                                  if not f.get('sometimes')]

            if mandatory_failures:
                msg += "(Expected errors not found: %s) " % mandatory_failures
                result = Result.FAILED
        elif self.expected_failures:
                msg += '%s(Expected errors occured: %s)%s' % (Colors.OKBLUE,
                                                           self.expected_failures,
                                                           Colors.ENDC)

        if result == Result.PASSED:
            if self._check_checksum():
                msg += "%s(Checksum matched: vpg_md5: %s otc_md5: %s)%s" % \
                                                            (Colors.OKBLUE,
                                                            self.vpg_md5,
                                                            self.otc_md5,
                                                            Colors.ENDC)
            else:
                result = Result.FAILED
                msg += "%s(Checksum NOT matched: vpg_md5: %s otc_md5: %s)%s" % \
                                                            (Colors.FAIL,
                                                            self.vpg_md5,
                                                            self.otc_md5,
                                                            Colors.ENDC)

        self.set_result(result, msg.strip())


class GstValidateVaapiDecoderTestsGenerator(GstValidatePipelineTestsGenerator):
    def __init__(self, test_manager):
        GstValidatePipelineTestsGenerator.__init__(
            self, "vaapidecode", test_manager,
            "filesrc location=%(filename)s ! " + \
            "%(parser)s ! %(decoder)s ! %(vpp)s ! %(sink)s")

    def _clean_caps(self, caps):
        """
        Returns a list of key=value or structure name, without "(types)" or ";" or ","
        """
        return re.sub(r"\(.+?\)\s*| |;", '', caps).split(',')

    def _decide_elements(self, codec, container):

        demuxer = parser = decoder = None

        self.debug("codec: %s, container: %s", codec, container)

        #TODO put container name more
        if container:
            if "webm" in container:
                demuxer = "matroskademux"
            elif "quicktime" in container:
                demuxer = "qtdemux"

        #TODO put codec name more
        if (codec == "h264"):
            parser = "h264parse"
            decoder = "vaapih264dec"
        elif (codec == "h265"):
            parser = "h265parse"
            decoder = "vaapih265dec"
        elif (codec == "vp8"):
            if container:
                decoder = "vaapivp8dec"
            else:
                parser = "ivfparse"
                decoder = "vaapivp8dec"
        elif (codec == "vp9"):
            if container:
                decoder = "vp9dec"
            else:
                parser = "ivfparse"
                decoder = "vp9dec"
        else:
            self.error ("Error: couldn't recognize codec : %s/%s", codec, container)

        return demuxer, parser, decoder

    def _workaround_get_codec(self, filename):
        #TODO if gst can't get codec
        if "vp9.webm" in filename:
            return "vp9"

    def _get_reference_file(self, filename):
        #TODO return exact checksum file path
        """ For example,
        if "vp9" in filename:
            return "/PATH/vpg_md5.ref"
        elif "HEVC/JVT" in filename:
            return "/PATH/HEVC_JVT"
        elif "HEVC/HEVC_Allegro10.0" in filename:
            return "/PATH/HEVC_Allegro10.0"
        elif "HEVC/MSFT" in filename:
            return "/PATH/HEVC_MSFT"
        elif "HEVC/BBC" in filename:
            return "/PATH/HEVC_BBC_i"
        """
        return "/home/zzoon/gst/master/test-data/intelqa/decode/medias/checksum.txt"

    def populate_tests(self, uri_minfo_special_scenarios, scenarios):
        for uri, minfo, special_scenarios in uri_minfo_special_scenarios:
            protocol, filename = uri.split("://", 1);
            basename = os.path.basename(filename)
            container_caps = minfo.media_descriptor.get_caps()
            all_tracks_caps = minfo.media_descriptor.get_tracks_caps()
            codec = None

            for track_type, caps in all_tracks_caps:
                ccaps = self._clean_caps(caps)
                if track_type == "video":
                    codec = ccaps[0].split("/")[1]
                    codec = codec.split("-")[1]
                    break

            if container_caps:
                container = container_caps.split("/")[1]
            else:
                container = None

            # Workaround if it can't recognize codec
            if codec == None:
                codec = self._workaround_get_codec(filename)

            demuxer, parser, decoder = self._decide_elements(codec, container)

            if demuxer and parser:
                parser = demuxer + " ! " + parser
            elif demuxer and parser == None:
                parser = demuxer

            pipe_arguments = {
                "filename": filename,
                "parser": parser,
                "decoder": decoder,
                "vpp": "vaapipostproc",
                "sink": "checksumsink2 frame-checksum=FALSE file-checksum=TRUE plane-checksum=FALSE"
            }

            pipe = self._pipeline_template % pipe_arguments
            self.info ("pipeline : %s", pipe)

            classname = "vaapi.decoder.%s.%s" % (codec, basename)
            checksum_file = self._get_reference_file(filename)

            self.add_test(GstValidateVaapiTest(classname,
                                            self.test_manager.options,
                                            self.test_manager.reporter,
                                            pipe,
                                            filename,
                                            checksum_file))

class GstVaapiTestManager(GstValidateTestManager):

    name = "vaapi"

    def add_options(self, parser):
        group = parser.add_argument_group("GstValidate VA-API tools specific options"
                                          " and behaviours",
                                          description=None)

def setup_tests(test_manager, options):
    print("Setting up Gstreamer VA-API default tests")

    #TODO put exact directory path that you want
    options.add_paths("/home/zzoon/gst/master/test-data/intelqa/decode/medias")
    #options.add_paths(os.path.abspath(os.path.join(os.path.dirname(__file__), "medias")))

    pipelines_descriptions = []

    test_manager.add_generators(GstValidateVaapiDecoderTestsGenerator(test_manager))
    return True
