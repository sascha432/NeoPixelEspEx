import re
from os import path
import click
import subprocess
from SCons.Script import ARGUMENTS
from pprint import pprint
Import("env", "projenv")

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

def mem_analyzer(source, target, env):
    # https://github.com/Sermus/ESP8266_memory_analyzer
    exe = path.abspath(path.join(env.subst("$PROJECT_DIR"), "./scripts/MemAnalyzer.exe"))
    if not path.isfile(exe):
        click.secho("%s not found. Check if file exists or has a different name" % exe, fg="yellow")
        return
    args = [
        exe,
        which('xtensa-lx106-elf-objdump.exe', env)[0],
        path.realpath(str(target[0]))
    ]
    p = subprocess.Popen(args, text=True)
    p.wait()

env.AlwaysBuild(env.Alias('disasm', [env['PIOMAINPROG']], disassemble))
env.AlwaysBuild(env.Alias('disassemble', [env['PIOMAINPROG']], disassemble))

env.AddPostAction('$BUILD_DIR/${PROGNAME}.elf', mem_analyzer)
