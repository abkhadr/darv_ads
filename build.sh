#!/bin/bash

# build.sh - سكريبت بناء وتشغيل DARV

echo "╔════════════════════════════════════════╗"
echo "║      DARV Build Script                 ║"
echo "║      نظام التطوير الذاتي الدائري      ║"
echo "╚════════════════════════════════════════╝"
echo ""

# ألوان للإخراج
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# التحقق من وجود CMake
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}[ERROR]${NC} CMake غير مثبت!"
    echo "قم بتثبيته أولاً:"
    echo "  Ubuntu/Debian: sudo apt-get install cmake"
    echo "  macOS: brew install cmake"
    exit 1
fi

# التحقق من وجود مجلد core
if [ ! -d "core" ]; then
    echo -e "${RED}[ERROR]${NC} مجلد core غير موجود!"
    echo "تأكد من تشغيل السكريبت من مجلد المشروع الرئيسي"
    exit 1
fi

# إنشاء البنية الأساسية
echo -e "${YELLOW}[STEP 1/5]${NC} إنشاء هيكل المشروع..."

# إنشاء المجلدات
mkdir -p memory/history
mkdir -p projects/example_project/build
mkdir -p build

echo -e "${GREEN}✓${NC} تم إنشاء المجلدات"

# إنشاء المشروع المثال إذا لم يكن موجود
if [ ! -f "projects/example_project/main.cpp" ]; then
    echo -e "${YELLOW}[STEP 2/5]${NC} إنشاء المشروع المثال..."
    
    cat > projects/example_project/main.cpp << 'EOF'
#include <iostream>
#include <chrono>

int main() {
    auto start = std::chrono::high_resolution_clock::now();
    
    long long sum = 0;
    for (int i = 0; i < 10000000; i++) {
        sum += i;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    
    std::cout << "المجموع: " << sum << std::endl;
    std::cout << "الوقت المستغرق: " << elapsed.count() << " ms" << std::endl;
    
    return 0;
}
EOF

    cat > projects/example_project/CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(example)

set(CMAKE_CXX_STANDARD 11)

add_executable(example main.cpp)
EOF
    
    echo -e "${GREEN}✓${NC} تم إنشاء المشروع المثال"
else
    echo -e "${YELLOW}[STEP 2/5]${NC} المشروع المثال موجود مسبقاً"
fi

# بناء DARV
echo -e "${YELLOW}[STEP 3/5]${NC} بناء DARV..."
cd build

if cmake .. && make; then
    echo -e "${GREEN}✓${NC} تم بناء DARV بنجاح"
else
    echo -e "${RED}✗${NC} فشل بناء DARV"
    exit 1
fi

cd ..

# بناء المشروع المثال (المرة الأولى)
echo -e "${YELLOW}[STEP 4/5]${NC} بناء المشروع المثال..."
cd projects/example_project/build

if cmake .. && make; then
    echo -e "${GREEN}✓${NC} تم بناء المشروع المثال"
else
    echo -e "${RED}✗${NC} فشل بناء المشروع المثال"
    exit 1
fi

cd ../../..

# تشغيل DARV
echo -e "${YELLOW}[STEP 5/5]${NC} تشغيل DARV..."
echo ""
echo "════════════════════════════════════════"
echo ""

./build/darv

echo ""
echo "════════════════════════════════════════"
echo -e "${GREEN}✓ اكتمل التنفيذ!${NC}"
echo ""
echo "يمكنك مراجعة:"
echo "  - السجلات: ./memory/cycles.log"
echo "  - التحسينات: ./memory/history/"
echo ""