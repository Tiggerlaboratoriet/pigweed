# Copyright 2024 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
"""Rules for processing binary executables."""

load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain", "use_cpp_toolchain")
load("//pw_toolchain/action:action_names.bzl", "PW_ACTION_NAMES")

def _run_action_on_executable(
        ctx,
        action_name,
        action_args,
        input,
        output,
        additional_outputs,
        output_executable = False):
    """Macro to be used in rule implementation to run an action on input executable.

    Looks up the current toolchain to find the path to the specified action."""
    cc_toolchain = find_cpp_toolchain(ctx)

    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )
    tool_path = cc_common.get_tool_for_action(
        feature_configuration = feature_configuration,
        action_name = action_name,
    )

    ctx.actions.run_shell(
        inputs = depset(
            direct = [input],
            transitive = [
                cc_toolchain.all_files,
            ],
        ),
        outputs = [output],
        command = "{tool} {args}".format(
            tool = tool_path,
            args = action_args,
        ),
    )

    return DefaultInfo(
        files = depset([output] + additional_outputs),
        executable = output if output_executable else None,
    )

def _pw_elf_to_bin_impl(ctx):
    return _run_action_on_executable(
        ctx = ctx,
        # TODO: https://github.com/bazelbuild/rules_cc/issues/292 - Add a helper
        # to rules cc to make it possible to get this from ctx.attr._objcopy.
        action_name = ACTION_NAMES.objcopy_embed_data,
        action_args = "{args} {input} {output}".format(
            args = "-Obinary",
            input = ctx.executable.elf_input.path,
            output = ctx.outputs.bin_out.path,
        ),
        input = ctx.executable.elf_input,
        output = ctx.outputs.bin_out,
        additional_outputs = ctx.files.elf_input,
        output_executable = True,
    )

pw_elf_to_bin = rule(
    implementation = _pw_elf_to_bin_impl,
    doc = """Takes in an ELF executable and uses the toolchain objcopy tool to
    create a binary file, not containing any ELF headers. This can be used to
    create a bare-metal bootable image.
    """,
    attrs = {
        "bin_out": attr.output(mandatory = True),
        "elf_input": attr.label(mandatory = True, executable = True, cfg = "target"),
        "_objcopy": attr.label(
            default = "@rules_cc//cc/toolchains/actions:objcopy_embed_data",
        ),
    },
    executable = True,
    toolchains = use_cpp_toolchain(),
    fragments = ["cpp"],
)

def _pw_elf_to_dump_impl(ctx):
    return _run_action_on_executable(
        ctx = ctx,
        # TODO: https://github.com/bazelbuild/rules_cc/issues/292 - Add a helper
        # to rules cc to make it possible to get this from ctx.attr._objdump.
        action_name = PW_ACTION_NAMES.objdump_disassemble,
        action_args = "{args} {input} > {output}".format(
            args = "-dx",
            input = ctx.executable.elf_input.path,
            output = ctx.outputs.dump_out.path,
        ),
        input = ctx.executable.elf_input,
        output = ctx.outputs.dump_out,
        additional_outputs = ctx.files.elf_input,
    )

pw_elf_to_dump = rule(
    implementation = _pw_elf_to_dump_impl,
    doc = """Takes in an ELF executable and uses the toolchain objdump tool to
    create a text file dump of the contents.
    """,
    attrs = {
        "dump_out": attr.output(mandatory = True),
        "elf_input": attr.label(mandatory = True, executable = True, cfg = "target"),
        "_objdump": attr.label(
            default = "//pw_toolchain/action:objdump_disassemble",
        ),
    },
    toolchains = use_cpp_toolchain(),
    fragments = ["cpp"],
)
