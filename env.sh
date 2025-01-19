CUR_DIR=`pwd`
ROOT=`dirname ${BASH_SOURCE[0]:-${(%):-%x}}`
cd $ROOT; ROOT=`pwd`; cd $CUR_DIR
ROOT=$ROOT/install
echo "JSI-Toolkit Install Directory: $ROOT"
export PATH=$ROOT/bin:$PATH
export LD_LIBRARY_PATH=$ROOT/lib:$LD_LIBRARY_PATH
