#ifndef DARV_CYCLE_HPP
#define DARV_CYCLE_HPP

#include "darv_types.hpp"
#include "darv_executor.hpp"
#include "darv_evaluator.hpp"
#include "darv_improver.hpp"
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <sys/stat.h>

namespace darv {

class DarvCycle {
private:
    Executor executor;
    Evaluator evaluator;
    Improver improver;
    
    std::vector<CycleRecord> history;
    ProjectConfig config;
    std::string memory_path;
    
    // إنشاء مجلد إذا لم يكن موجود
    void ensure_directory(const std::string& path) {
        struct stat info;
        if (stat(path.c_str(), &info) != 0) {
            mkdir(path.c_str(), 0755);
        }
    }
    
    // حفظ سجل الدورة
    void save_cycle_record(const CycleRecord& record) {
        std::string log_file = memory_path + "/cycles.log";
        std::ofstream log(log_file, std::ios::app);
        
        auto time = std::chrono::system_clock::to_time_t(record.timestamp);
        
        log << "\n========================================\n";
        log << "دورة رقم: " << record.cycle_number << "\n";
        log << "الوقت: " << std::ctime(&time);
        log << "النتيجة: " << record.evaluation.overall_score << "/100\n";
        log << "وقت التنفيذ: " << record.execution.execution_time_ms << " ms\n";
        log << "التحسينات المطبقة: " << record.improvements.size() << "\n";
        log << "ملاحظات: " << record.notes << "\n";
        log << "========================================\n";
        
        log.close();
    }
    
    // التحقق من التقارب (Convergence)
    bool check_convergence() {
        if (history.size() < 3) {
            return false;
        }
        
        // إذا آخر 3 دورات لم تتحسن بشكل ملحوظ
        double last_score = history[history.size() - 1].evaluation.overall_score;
        double prev_score = history[history.size() - 2].evaluation.overall_score;
        double prev_prev_score = history[history.size() - 3].evaluation.overall_score;
        
        double improvement1 = last_score - prev_score;
        double improvement2 = prev_score - prev_prev_score;
        
        // إذا التحسين أقل من threshold في آخر دورتين
        if (std::abs(improvement1) < config.convergence_threshold &&
            std::abs(improvement2) < config.convergence_threshold) {
            return true;
        }
        
        // إذا وصلنا لنتيجة ممتازة (>95)
        if (last_score > 95.0) {
            return true;
        }
        
        return false;
    }

public:
    DarvCycle(const ProjectConfig& proj_config, const std::string& mem_path) 
        : config(proj_config), memory_path(mem_path) {
        
        // إنشاء المجلدات الضرورية
        ensure_directory(memory_path);
        ensure_directory(memory_path + "/history");
    }
    
    // تشغيل دورة واحدة
    CycleRecord run_single_cycle(int cycle_number) {
        std::cout << "\n╔════════════════════════════════════════╗\n";
        std::cout << "║   دورة DARV رقم: " << std::setw(2) << cycle_number << "               ║\n";
        std::cout << "╚════════════════════════════════════════╝\n";
        
        CycleRecord record;
        record.cycle_number = cycle_number;
        record.timestamp = std::chrono::system_clock::now();
        
        // الخطوة 1: البناء
        std::cout << "\n► الخطوة 1/4: البناء (Build)\n";
        ExecutionResult build_result = executor.build_project(config);
        
        if (!build_result.success) {
            record.execution = build_result;
            record.evaluation.overall_score = 0.0;
            record.evaluation.needs_improvement = true;
            record.notes = "فشل البناء";
            return record;
        }
        
        // الخطوة 2: التنفيذ
        std::cout << "\n► الخطوة 2/4: التنفيذ (Execute)\n";
        ExecutionResult run_result = executor.run_project(config);
        record.execution = run_result;
        
        // الخطوة 3: التقييم
        std::cout << "\n► الخطوة 3/4: التقييم (Evaluate)\n";
        QualityEvaluation evaluation = evaluator.evaluate(run_result);
        record.evaluation = evaluation;
        
        // الخطوة 4: التحسين
        std::cout << "\n► الخطوة 4/4: التحسين (Improve)\n";
        std::vector<Improvement> improvements = 
            improver.generate_improvements(evaluation, config);
        record.improvements = improvements;
        
        if (!improvements.empty()) {
            bool applied = improver.apply_improvements(
                improvements, config, memory_path + "/history");
            record.applied_improvements = applied;
            
            if (applied) {
                record.notes = "تم تطبيق " + std::to_string(improvements.size()) + " تحسين";
            } else {
                record.notes = "فشل تطبيق بعض التحسينات";
            }
        } else {
            record.applied_improvements = false;
            record.notes = "لا يوجد تحسينات مطلوبة";
        }
        
        // حفظ السجل
        save_cycle_record(record);
        
        return record;
    }
    
    // تشغيل عدة دورات متتالية
    void run_cycles() {
        std::cout << "\n";
        std::cout << "██████╗  █████╗ ██████╗ ██╗   ██╗\n";
        std::cout << "██╔══██╗██╔══██╗██╔══██╗██║   ██║\n";
        std::cout << "██║  ██║███████║██████╔╝██║   ██║\n";
        std::cout << "██║  ██║██╔══██║██╔══██╗╚██╗ ██╔╝\n";
        std::cout << "██████╔╝██║  ██║██║  ██║ ╚████╔╝ \n";
        std::cout << "╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝  ╚═══╝  \n";
        std::cout << "\nنظام التطوير الذاتي الدائري\n";
        std::cout << "المشروع: " << config.name << "\n";
        std::cout << "الحد الأقصى للدورات: " << config.max_cycles << "\n\n";
        
        for (int i = 1; i <= config.max_cycles; i++) {
            CycleRecord record = run_single_cycle(i);
            history.push_back(record);
            
            // طباعة ملخص
            std::cout << "\n═══ ملخص الدورة " << i << " ═══\n";
            std::cout << "النتيجة: " << record.evaluation.overall_score << "/100\n";
            std::cout << "الوقت: " << record.execution.execution_time_ms << " ms\n";
            std::cout << "الملاحظات: " << record.notes << "\n";
            
            // التحقق من التقارب
            if (check_convergence()) {
                std::cout << "\n✓ تم الوصول للتقارب (Convergence)!\n";
                std::cout << "النظام توقف بعد " << i << " دورة\n";
                break;
            }
            
            // انتظار قصير بين الدورات
            std::cout << "\n⏳ انتظار قبل الدورة التالية...\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        
        print_summary();
    }
    
    // طباعة ملخص شامل
    void print_summary() {
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════╗\n";
        std::cout << "║       ملخص شامل لجميع الدورات        ║\n";
        std::cout << "╚════════════════════════════════════════╝\n\n";
        
        std::cout << "عدد الدورات المنفذة: " << history.size() << "\n\n";
        
        for (size_t i = 0; i < history.size(); i++) {
            const auto& rec = history[i];
            std::cout << "دورة " << (i+1) << ": ";
            std::cout << "النتيجة=" << rec.evaluation.overall_score << " | ";
            std::cout << "الوقت=" << rec.execution.execution_time_ms << "ms | ";
            std::cout << "التحسينات=" << rec.improvements.size() << "\n";
        }
        
        if (!history.empty()) {
            double first_score = history.front().evaluation.overall_score;
            double last_score = history.back().evaluation.overall_score;
            double improvement = last_score - first_score;
            
            std::cout << "\n═══ الإحصائيات ═══\n";
            std::cout << "النتيجة الأولى: " << first_score << "/100\n";
            std::cout << "النتيجة النهائية: " << last_score << "/100\n";
            std::cout << "التحسن الكلي: " << improvement << " نقطة\n";
        }
        
        std::cout << "\n✓ انتهت جميع الدورات بنجاح!\n";
    }
    
    // الحصول على التاريخ
    const std::vector<CycleRecord>& get_history() const {
        return history;
    }
};

} // namespace darv

#endif // DARV_CYCLE_HPP