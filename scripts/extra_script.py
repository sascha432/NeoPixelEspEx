import re
from os import path
import os
import click
import subprocess
from SCons.Script import ARGUMENTS
from pprint import pprint
Import("env", "projenv")

def which(name, env, flags=os.F_OK):
    result = []
    extensions = env["ENV"]["PATHEXT"].split(os.pathsep)
    if not extensions:
        extensions = [".exe", ""]

    for path in env["ENV"]["PATH"].split(os.pathsep):
        path = os.path.join(path, name)
        if os.access(path, flags):
            result.append(os.path.normpath(path))
        for ext in extensions:
            whole = path + ext
            if os.access(whole, os.X_OK):
                result.append(os.path.normpath(whole))
    return result

def disassemble(source, target, env):

    verbose = int(ARGUMENTS.get("PIOVERBOSE", 0))

    source = path.abspath(env.subst('$PIOMAINPROG'))
    target = env.subst(env.GetProjectOption('custom_disassemble_target', '$BUILD_DIR/${PROGNAME}.lst'))
    target = path.abspath(target)

    command = env.subst(env.GetProjectOption('custom_disassemble_bin', env['CC'].replace('gcc', 'objdump')))

    options = re.split(r'[\s]', env.subst(env.GetProjectOption('custom_disassemble_options', '-S -C')))

    args = [command] + options + [source, '>', target]
    if verbose:
        click.echo(' '.join(args))

    return_code = subprocess.run(args, shell=True).returncode
    if return_code != 0:
        click.secho('failed to run: %s' % ' '.join(args))
        env.Exit(1)

    click.echo('-' * click.get_terminal_size()[0])
    click.secho('Created: ', fg='yellow', nl=False)
    click.secho(target)

env.AlwaysBuild(env.Alias('disasm', [env['PIOMAINPROG']], disassemble))
env.AlwaysBuild(env.Alias('disassemble', [env['PIOMAINPROG']], disassemble))
