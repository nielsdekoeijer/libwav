cd src
find . -name "*.cpp" -exec clang-format -i {} +
find . -name "*.hpp" -exec clang-format -i {} +
cd -

cd include 
find . -name "*.cpp" -exec clang-format -i {} +
find . -name "*.hpp" -exec clang-format -i {} +
cd -
