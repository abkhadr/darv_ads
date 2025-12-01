#ifndef DARV_IMPROVER_HPP
#define DARV_IMPROVER_HPP

#include "darv_types.hpp"
#include <fstream>
#include <iostream>
#include <regex>

namespace darv {

class Improver {
private:
    int improvement_counter = 0;
    
    // قراءة ملف
    std::string read_file(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return "";
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    // كتابة ملف
    bool write_file(const std::string& filepath, const std::string& content) {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        
        file << content;
        return true;
    }
    
    // توليد تحسينات بسيطة (Rule-based للنسخة الأولى)
    std::vector<Improvement> generate_simple_improvements(
        const QualityEvaluation& eval,
        const ProjectConfig& config) {
        
        std::vector<Improvement> improvements;
        
        // تحسين 1: إضافة -O2 flag إذا كان بطيء
        if (eval.metrics.avg_execution_time > 1000.0) {
            Improvement imp;
            imp.description = "إضافة optimization flag -O2 لتسريع التنفيذ";
            imp.target_file = "CMakeLists.txt";
            imp.expected_impact = 0.7;
            imp.priority = 8;
            imp.patch_content = R"(
# إضافة optimization flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
)";
            improvements.push_back(imp);
        }
        
        // تحسين 2: إصلاح التحذيرات
        if (eval.metrics.warning_count > 0) {
            Improvement imp;
            imp.description = "تفعيل المزيد من التحذيرات لتحسين جودة الكود";
            imp.target_file = "CMakeLists.txt";
            imp.expected_impact = 0.3;
            imp.priority = 5;
            imp.patch_content = R"(
# تفعيل المزيد من التحذيرات
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
)";
            improvements.push_back(imp);
        }
        
        // تحسين 3: استخدام C++17 إذا لم يكن محدد
        Improvement imp;
        imp.description = "استخدام C++17 للاستفادة من ميزات حديثة";
        imp.target_file = "CMakeLists.txt";
        imp.expected_impact = 0.2;
        imp.priority = 3;
        imp.patch_content = R"(
# استخدام C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
)";
        improvements.push_back(imp);
        
        return improvements;
    }

public:
    Improver() = default;
    
    // توليد تحسينات بناءً على التقييم
    std::vector<Improvement> generate_improvements(
        const QualityEvaluation& evaluation,
        const ProjectConfig& config) {
        
        std::cout << "\n[IMPROVER] توليد تحسينات..." << std::endl;
        
        if (!evaluation.needs_improvement) {
            std::cout << "[IMPROVER] لا يوجد حاجة للتحسين - الجودة ممتازة!" << std::endl;
            return {};
        }
        
        // في النسخة الأولى نستخدم تحسينات بسيطة
        auto improvements = generate_simple_improvements(evaluation, config);
        
        std::cout << "[IMPROVER] تم توليد " << improvements.size() << " تحسين" << std::endl;
        
        for (const auto& imp : improvements) {
            std::cout << "  - " << imp.description 
                      << " (الأولوية: " << imp.priority << ")" << std::endl;
        }
        
        return improvements;
    }
    
    // تطبيق التحسينات
    bool apply_improvements(
        const std::vector<Improvement>& improvements,
        const ProjectConfig& config,
        const std::string& history_path) {
        
        std::cout << "\n[IMPROVER] تطبيق التحسينات..." << std::endl;
        
        if (improvements.empty()) {
            return true;
        }
        
        // ترتيب التحسينات حسب الأولوية
        auto sorted_improvements = improvements;
        std::sort(sorted_improvements.begin(), sorted_improvements.end(),
                  [](const Improvement& a, const Improvement& b) {
                      return a.priority > b.priority;
                  });
        
        bool all_applied = true;
        
        for (const auto& imp : sorted_improvements) {
            std::cout << "[IMPROVER] تطبيق: " << imp.description << std::endl;
            
            std::string target = config.root_path + "/" + imp.target_file;
            
            // قراءة الملف الحالي
            std::string current_content = read_file(target);
            
            // إضافة التحسين
            std::string new_content = current_content + "\n" + imp.patch_content;
            
            // كتابة الملف الجديد
            if (write_file(target, new_content)) {
                std::cout << "[IMPROVER] تم التطبيق بنجاح ✓" << std::endl;
                
                // حفظ الـ patch في التاريخ
                improvement_counter++;
                std::string patch_file = history_path + "/patch_" + 
                                        std::to_string(improvement_counter) + ".diff";
                
                std::ofstream patch(patch_file);
                patch << "--- " << imp.target_file << "\n";
                patch << "+++ " << imp.target_file << "\n";
                patch << "@@ Improvement: " << imp.description << " @@\n";
                patch << imp.patch_content << "\n";
                patch.close();
                
            } else {
                std::cout << "[IMPROVER] فشل التطبيق ✗" << std::endl;
                all_applied = false;
            }
        }
        
        return all_applied;
    }
};

} // namespace darv

#endif // DARV_IMPROVER_HPP