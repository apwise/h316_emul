pipeline {
    agent any
    stages {

        stage('build') {
            steps {
                sh 'aclocal && autoconf && automake'
                sh 'mkdir -p build install && workspace=`pwd` && cd build && ../configure --prefix=${workspace}/install && make'
            }
        }
    }
