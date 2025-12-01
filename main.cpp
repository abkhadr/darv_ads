#include "core/darv_cycle.hpp"
#include <iostream>
#include <filesystem>

using namespace darv;

int main(int argc, char* argv[]) {
    std::cout << "مرحباً بك في DARV - نظام التطوير الذاتي الدائري\n\n";
    
    // إعداد المشروع
    ProjectConfig config;
    config.name = "example_project";
    config.root_path = "./projects/example_project";
    config.build_path = config.root_path + "/build";
    config.executable_name = "example";
    
    // أوامر البناء
    config.build_commands = {
        "cd " + config.root_path + " && mkdir -p build",
        "cd " + config.build_path + " && cmake ..",
        "cd " + config.build_path + " && make"
    };
    
    // أوامر التشغيل
    config.run_commands = {
        config.build_path + "/example"
    };
    
    // إعدادات الدورة
    config.max_cycles = 5;  // عدد الدورات القصوى
    config.convergence_threshold = 2.0;  // عتبة التقارب
    
    // إنشاء مجلد الذاكرة
    std::string memory_path = "./memory";
    std::filesystem::create_directories(memory_path);
    std::filesystem::create_directories(memory_path + "/history");
    
    // بدء دورة DARV
    try {
        DarvCycle cycle(config, memory_path);
        cycle.run_cycles();
        
        std::cout << "\n✓ انتهى DARV من العمل بنجاح!\n";
        std::cout << "يمكنك مراجعة السجلات في: " << memory_path << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "خطأ: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}