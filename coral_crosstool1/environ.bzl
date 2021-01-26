# generate a .bzl file with variable called PI_TOOLCHAIN_ROOT_DIR=$PI_TOOLCHAIN_ROOT_DIR (rhs is grabbed from bash env)

def _impl(repository_ctx):
  repository_ctx.file("pi_toolchain_root.bzl", "TOOLCHAIN_ROOT_DIR = \"%s\"" % \
    repository_ctx.os.environ.get("TOOLCHAIN_ROOT_DIR", "/home/dev/oosman/.leila/toolchains/raspberrypi0"))
  repository_ctx.file("BUILD", "")

pi_toolchain_repository = repository_rule(
    implementation=_impl,
    environ = ["PI_TOOLCHAIN_ROOT_DIR"],
)
