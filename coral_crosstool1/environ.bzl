# generate a .bzl file with variable called PI_TOOLCHAIN_ROOT_DIR=$PI_TOOLCHAIN_ROOT_DIR (rhs is grabbed from bash env)

def _impl(repository_ctx):
  print("############################# environ.bzl")
  repository_ctx.file("pi_toolchain_root.bzl", "PI_TOOLCHAIN_ROOT_DIR = \"%s\"" % \
    repository_ctx.os.environ.get("PI_TOOLCHAIN_ROOT_DIR", "/home/dev/oosman/.leila/toolchains/rpi"))
  repository_ctx.file("BUILD", "")

pi_toolchain_repository = repository_rule(
    implementation=_impl,
    environ = ["PI_TOOLCHAIN_ROOT_DIR"],
)

print("################ environ ###############")

