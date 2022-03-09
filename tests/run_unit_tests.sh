FOLDER=build_unittests
set -e

mkdir -p $FOLDER
pushd $FOLDER
cmake ../unit -DCMAKE_BUILD_TYPE=Debug
make -j4
ctest --verbose .
popd
