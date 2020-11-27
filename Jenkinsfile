pipeline {
  agent none
  stages {
    stage("buildall") {
      matrix {
        agent {
          label "${label}"
        }
        axes {
          axis {
            name "label"
            values "linux", "macos", "windows"
          }
          axis {
            name "compiler"
            values "gcc", "clang_llvm", "clang_stdc", "vc"
          }
        }
        stages {
          stage("get") {
            steps {
              sh "git clone https://github.com/nlitsme/cpputils submodules/cpputils || true"
            }
          }
          stage("build") {
            steps {
                sh '''
echo compiler=$compiler, label=$label
if [[ $label == windows ]]; then
. /c/local/msvcenv.sh
  make CXX=cl idasdk=c:/local/idasdk75
elif [[ $label == macos ]]; then
  make idasdk=$HOME/src/idasdk752
else
  make idasdk=$HOME/src/idasdk75
fi
./unittests
./idbtool | grep rootnode

'''
            }
          }
        }
      }
    }
  }
}
