pipeline {
    agent any
    stages {

        stage('build') {
            steps {
                sh 'aclocal && autoconf && automake --add-missing'
                sh 'mkdir -p build install'
                sh 'workspace=`pwd` && cd build && ../configure --prefix=${workspace}/install'
                sh 'cd build && make'
                sh 'cd build && make install'
            }
        }
        
        stage('test') {
            steps {
                sh 'cd tests/VT && ./VT.sh'
            }
        }
    }
    post {
        always {
            junit 'results/*.xml'
        }
    }
}
