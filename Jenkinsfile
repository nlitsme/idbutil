pipeline {
  agent { label "windows" }
  stages {
    stage("clean") { steps { sh '''git clean -dfx''' } }
    stage("windows-build") {
      steps { 
sh '''#!/bin/bash
set -e
. /c/local/msvcenv.sh
export BOOST_ROOT=c:/local/boost_1_74_0
export IDASDK=c:/local/idasdk_pro82
make vc
'''
      }
    }
  }
}
