"""Configure native CI with the active Python interpreter and external GoogleTest."""
import pathlib,subprocess,sys
subprocess.run(['cmake','-S','.','-B','build','-DCMAKE_BUILD_TYPE=Release',
    '-DSSM_GTEST_SOURCE='+str(pathlib.Path('.deps/googletest').resolve()),
    '-DSSM_REFERENCE_PYTHON='+sys.executable],check=True)
