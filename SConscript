import rtconfig
from building import *

cwd     = GetCurrentDir()
src     = Split("""
./src/qf/qep_hsm.c
./src/qf/qep_msm.c
./src/qf/qf_act.c
./src/qf/qf_defer.c
./src/qf/qf_dyn.c
./src/qf/qf_ps.c
./src/qf/qf_qact.c
./src/qf/qf_qeq.c
./src/qf/qf_qmact.c
./src/qf/qf_time.c
./include/qstamp.c
./ports/rt-thread/qf_hooks.c
./ports/rt-thread/qf_port.c
""")

if GetDepend(['QPC_USING_BLINKY_EXAMPLE']):
    src += Glob('examples/rt-thread/blinky/blinky.c')

path = [cwd + "/ports/rt-thread", cwd + "/include", cwd + "/src"]

group = DefineGroup('qpc', src, depend = ['RT_USING_MAILBOX', 'PKG_USING_QPC'], CPPPATH = path)

Return('group')