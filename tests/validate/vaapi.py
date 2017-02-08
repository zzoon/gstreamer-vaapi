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

TEST_MANAGER = "validate"

def get_pipeline_description(idx):
    if idx is 0:
      return ("vaapi_pipeline", "videotestsrc ! vaapih264enc ! vaapih264dec ! vaapisink")
    elif idx is 1:
      return ("vaapi_encoder", "videotestsrc ! vaapih264enc ! fakesink")

def setup_tests(test_manager, options):
    print("Setting up Gstreamer VA-API default tests")

    options.add_paths(os.path.abspath(os.path.join(os.path.dirname(__file__), "medias")))

    pipelines_descriptions = []
    valid_scenarios = ["play_15s"]
    test_manager.add_scenarios(valid_scenarios)

    for i in range(2):
        description = get_pipeline_description(i)
        pipelines_descriptions.append(description)

    test_manager.add_generators(test_manager.GstValidatePipelineTestsGenerator
                                ("vaapi_validate", test_manager,
                                pipelines_descriptions=pipelines_descriptions,
                                valid_scenarios=valid_scenarios))

    test_manager.add_generators(test_manager.GstValidatePlaybinTestsGenerator(test_manager))
    
    return True
