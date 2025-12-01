#ifndef DARV_EVALUATOR_HPP
#define DARV_EVALUATOR_HPP

#include "darv_types.hpp"
#include <algorithm>
#include <numeric>
#include <iostream>
#include <sstream>

namespace darv {

class Evaluator {
private:
    std::vector<double> execution_history;
    
    // تحليل الأخطاء في النتيجة
    std::vector<std::string> analyze_errors(const ExecutionResult& result) {
        std::vector<std::string> errors;
        
        if (!result.success) {
            errors.push_back("فشل التنفيذ - كود الخروج: " + std::to_string(result.exit_code));
        }
        
        // البحث عن كلمات مفتاحية تدل على أخطاء
        std::vector<std::string> error_keywords = {
            "error", "Error", "ERROR",
            "exception", "Exception",
            "segmentation fault", "core dumped",
            "undefined reference", "cannot find"
        };
        
        for (const auto& keyword : error_keywords) {
            if (result.stderr_output.find(keyword) != std::string::npos ||
                result.stdout_output.find(keyword) != std::string::npos) {
                errors.push_back("وُجد خطأ: " + keyword);
            }
        }
        
        return errors;
    }
    
    // تحليل التحذيرات
    std::vector<std::string> analyze_warnings(const ExecutionResult& result) {
        std::vector<std::string> warnings;
        
        std::vector<std::string> warning_keywords = {
            "warning", "Warning", "WARNING",
            "deprecated", "Deprecated"
        };
        
        for (const auto& keyword : warning_keywords) {
            if (result.stderr_output.find(keyword) != std::string::npos ||
                result.stdout_output.find(keyword) != std::string::npos) {
                warnings.push_back("وُجد تحذير: " + keyword);
            }
        }
        
        return warnings;
    }
    
    // حساب النقاط بناءً على الأداء
    double calculate_score(const PerformanceMetrics& metrics) {
        double score = 100.0;
        
        // خصم نقاط على الأخطاء
        score -= metrics.error_count * 20;
        score -= metrics.warning_count * 5;
        
        // خصم نقاط على البطء (إذا أكثر من 1000ms)
        if (metrics.avg_execution_time > 1000.0) {
            score -= (metrics.avg_execution_time - 1000.0) / 100.0;
        }
        
        // التأكد من أن النقاط بين 0-100
        return std::max(0.0, std::min(100.0, score));
    }

public:
    Evaluator() = default;
    
    // تقييم نتيجة التنفيذ
    QualityEvaluation evaluate(const ExecutionResult& result) {
        std::cout << "\n[EVALUATOR] تقييم النتيجة..." << std::endl;
        
        QualityEvaluation evaluation;
        
        // حفظ وقت التنفيذ في التاريخ
        execution_history.push_back(result.execution_time_ms);
        
        // حساب المقاييس
        evaluation.metrics.avg_execution_time = result.execution_time_ms;
        
        if (execution_history.size() > 1) {
            evaluation.metrics.avg_execution_time = 
                std::accumulate(execution_history.begin(), execution_history.end(), 0.0) 
                / execution_history.size();
            
            evaluation.metrics.min_execution_time = 
                *std::min_element(execution_history.begin(), execution_history.end());
            
            evaluation.metrics.max_execution_time = 
                *std::max_element(execution_history.begin(), execution_history.end());
        } else {
            evaluation.metrics.min_execution_time = result.execution_time_ms;
            evaluation.metrics.max_execution_time = result.execution_time_ms;
        }
        
        // تحليل الأخطاء والتحذيرات
        auto errors = analyze_errors(result);
        auto warnings = analyze_warnings(result);
        
        evaluation.metrics.error_count = errors.size();
        evaluation.metrics.warning_count = warnings.size();
        
        // إضافة المشاكل والاقتراحات
        evaluation.issues.insert(evaluation.issues.end(), errors.begin(), errors.end());
        evaluation.issues.insert(evaluation.issues.end(), warnings.begin(), warnings.end());
        
        // اقتراحات التحسين
        if (result.execution_time_ms > 1000.0) {
            evaluation.suggestions.push_back("الأداء بطيء - يمكن تحسين السرعة");
        }
        
        if (!result.success) {
            evaluation.suggestions.push_back("يجب إصلاح الأخطاء أولاً");
        }
        
        if (warnings.size() > 0) {
            evaluation.suggestions.push_back("يُفضل إصلاح التحذيرات");
        }
        
        // حساب النقاط الكلية
        evaluation.overall_score = calculate_score(evaluation.metrics);
        
        // تحديد هل يحتاج تحسين
        evaluation.needs_improvement = (evaluation.overall_score < 80.0) || 
                                       (evaluation.metrics.error_count > 0);
        
        // طباعة التقييم
        std::cout << "[EVALUATOR] النتيجة: " << evaluation.overall_score << "/100" << std::endl;
        std::cout << "[EVALUATOR] الأخطاء: " << evaluation.metrics.error_count << std::endl;
        std::cout << "[EVALUATOR] التحذيرات: " << evaluation.metrics.warning_count << std::endl;
        std::cout << "[EVALUATOR] وقت التنفيذ: " << evaluation.metrics.avg_execution_time << " ms" << std::endl;
        std::cout << "[EVALUATOR] يحتاج تحسين: " << (evaluation.needs_improvement ? "نعم" : "لا") << std::endl;
        
        return evaluation;
    }
    
    // مسح التاريخ
    void clear_history() {
        execution_history.clear();
    }
};

} // namespace darv

#endif // DARV_EVALUATOR_HPP