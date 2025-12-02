#include "core/darv_tensor.hpp"
#include "core/darv_dataset.hpp"
#include "core/darv_optimizer.hpp"
#include <iostream>
#include <iomanip>

using namespace darv::autograd;

void print_separator(const std::string& title) {
    std::cout << "\n╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║ " << std::left << std::setw(55) << title << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n\n";
}

// Test 1: Basic Tensor Operations
void test_tensor_operations() {
    print_separator("Test 1: العمليات الأساسية على Tensors");
    
    // Create tensors
    auto a = Tensor::ones(Shape{3, 2});
    auto b = 2.0 * Tensor::ones(Shape{3, 2});  // استخدام operator الجديد
    
    a->set_name("a");
    b->set_name("b");
    
    std::cout << "Tensor a (3x2 of ones):\n";
    a->print("  ");
    
    std::cout << "\nTensor b (3x2 of twos):\n";
    b->print("  ");
    
    // Addition
    auto c = a + b;
    c->set_name("c = a + b");
    std::cout << "\nAddition:\n";
    c->print("  ");
    
    // Element-wise multiplication
    auto d = a * b;
    d->set_name("d = a * b");
    std::cout << "\nElement-wise multiplication:\n";
    d->print("  ");
    
    // Backward
    std::cout << "\nComputing gradients...\n";
    auto loss = c->sum();
    loss->backward();
    
    std::cout << "\nGradients after backward:\n";
    a->print("  a: ");
    b->print("  b: ");
}

// Test 2: Matrix Multiplication
void test_matmul() {
    print_separator("Test 2: Matrix Multiplication");
    
    // Create matrices
    std::vector<double> data_a = {1, 2, 3, 4, 5, 6};
    std::vector<double> data_b = {7, 8, 9, 10, 11, 12};
    
    auto A = std::make_shared<Tensor>(data_a, Shape{2, 3});  // 2x3
    auto B = std::make_shared<Tensor>(data_b, Shape{3, 2});  // 3x2
    
    A->set_name("A");
    B->set_name("B");
    
    std::cout << "Matrix A (2x3):\n";
    A->print("  ");
    
    std::cout << "\nMatrix B (3x2):\n";
    B->print("  ");
    
    // Matrix multiplication: C = A @ B
    auto C = A->matmul(B);
    C->set_name("C = A @ B");
    
    std::cout << "\nMatrix multiplication C = A @ B (2x2):\n";
    C->print("  ");
    
    // Backward
    std::cout << "\nComputing gradients...\n";
    auto loss = C->sum();
    loss->backward();
    
    std::cout << "\nGradients:\n";
    A->print("  A: ");
    B->print("  B: ");
}

// Test 3: Activation Functions
void test_activations() {
    print_separator("Test 3: Activation Functions");
    
    std::vector<double> data = {-2.0, -1.0, 0.0, 1.0, 2.0};
    auto x = std::make_shared<Tensor>(data, Shape{5});
    x->set_name("x");
    
    std::cout << "Input:\n";
    x->print("  ");
    
    // ReLU
    auto relu_out = x->relu();
    relu_out->set_name("ReLU(x)");
    std::cout << "\nReLU:\n";
    relu_out->print("  ");
    
    // Sigmoid
    auto sigmoid_out = x->sigmoid();
    sigmoid_out->set_name("Sigmoid(x)");
    std::cout << "\nSigmoid:\n";
    sigmoid_out->print("  ");
    
    // Tanh
    auto tanh_out = x->tanh_();
    tanh_out->set_name("Tanh(x)");
    std::cout << "\nTanh:\n";
    tanh_out->print("  ");
    
    // Test gradients
    std::cout << "\nTesting gradients for ReLU...\n";
    auto loss = relu_out->sum();
    loss->backward();
    x->print("  x gradient: ");
}

// Test 4: Simple Neural Network Training
void test_neural_network() {
    print_separator("Test 4: تدريب شبكة عصبية بسيطة");
    
    std::cout << "مشكلة: التنبؤ بـ y = 2x + 1\n\n";
    
    // Create dataset: y = 2x + 1
    std::vector<std::vector<double>> X = {
        {0.0}, {1.0}, {2.0}, {3.0}, {4.0}
    };
    std::vector<std::vector<double>> y = {
        {1.0}, {3.0}, {5.0}, {7.0}, {9.0}
    };
    
    auto dataset = create_dataset_from_vectors(X, y);
    std::cout << "Dataset:\n";
    for (size_t i = 0; i < dataset.size(); i++) {
        auto [data, label] = dataset[i];
        std::cout << "  x=" << data->data()[0] 
                  << " → y=" << label->data()[0] << "\n";
    }
    
    // Initialize parameters: y = wx + b
    auto w = Tensor::randn(Shape{1}, true);
    auto b = Tensor::randn(Shape{1}, true);
    
    std::cout << "\nInitial parameters:\n";
    std::cout << "  w = " << w->data()[0] << "\n";
    std::cout << "  b = " << b->data()[0] << "\n";
    
    // Training
    std::cout << "\nTraining...\n";
    double learning_rate = 0.01;
    int epochs = 100;
    
    for (int epoch = 0; epoch < epochs; epoch++) {
        double total_loss = 0.0;
        
        for (size_t i = 0; i < dataset.size(); i++) {
            auto [x_tensor, y_true] = dataset[i];
            
            // Forward: pred = w * x + b
            auto pred = w * x_tensor->data()[0];
            pred = pred + b;
            
            // Loss: MSE
            auto diff = pred + (-1.0 * y_true);  // استخدام operator الجديد
            auto loss = (diff * diff)->mean();
            
            // Backward
            w->zero_grad();
            b->zero_grad();
            loss->backward();
            
            // Update
            w->data()[0] -= learning_rate * w->grad()[0];
            b->data()[0] -= learning_rate * b->grad()[0];
            
            total_loss += loss->data()[0];
        }
        
        if (epoch % 20 == 0) {
            std::cout << "Epoch " << std::setw(3) << epoch 
                      << " | Loss: " << std::fixed << std::setprecision(4) 
                      << total_loss / dataset.size() << "\n";
        }
    }
    
    std::cout << "\nFinal parameters:\n";
    std::cout << "  w = " << std::fixed << std::setprecision(4) 
              << w->data()[0] << " (expected: 2.0)\n";
    std::cout << "  b = " << b->data()[0] << " (expected: 1.0)\n";
    
    // Test
    std::cout << "\nTesting:\n";
    for (double x_val = 0.0; x_val <= 5.0; x_val += 1.0) {
        double pred = w->data()[0] * x_val + b->data()[0];
        double expected = 2.0 * x_val + 1.0;
        std::cout << "  x=" << x_val 
                  << " → pred=" << std::setprecision(2) << pred 
                  << " (expected=" << expected << ")\n";
    }
}

// Test 5: Advanced Optimizers
void test_optimizers() {
    print_separator("Test 5: اختبار Optimizers المتقدمة");
    
    // Simple optimization problem: minimize f(x) = x^2
    std::cout << "مشكلة: minimize f(x) = x^2\n";
    std::cout << "Optimal solution: x = 0\n\n";
    
    struct OptimizerTest {
        std::string name;
        std::unique_ptr<Optimizer> optimizer;
        std::shared_ptr<Tensor> x;
    };
    
    std::vector<OptimizerTest> tests;
    
    // SGD
    {
        auto x = std::make_shared<Tensor>(std::vector<double>{5.0}, Shape{1}, true);
        tests.push_back({"SGD", std::make_unique<SGD>(std::vector{x}, 0.1), x});
    }
    
    // SGD with momentum
    {
        auto x = std::make_shared<Tensor>(std::vector<double>{5.0}, Shape{1}, true);
        tests.push_back({"SGD+Momentum", std::make_unique<SGD>(std::vector{x}, 0.1, 0.9), x});
    }
    
    // Adam
    {
        auto x = std::make_shared<Tensor>(std::vector<double>{5.0}, Shape{1}, true);
        tests.push_back({"Adam", std::make_unique<Adam>(std::vector{x}, 0.1), x});
    }
    
    // RMSprop
    {
        auto x = std::make_shared<Tensor>(std::vector<double>{5.0}, Shape{1}, true);
        tests.push_back({"RMSprop", std::make_unique<RMSprop>(std::vector{x}, 0.1), x});
    }
    
    // Train each optimizer
    int iterations = 50;
    
    for (auto& test : tests) {
        std::cout << test.name << ":\n";
        
        for (int i = 0; i < iterations; i++) {
            // f(x) = x^2 باستخدام دالة pow الجديدة
            auto loss = test.x->pow(2);
            
            test.optimizer->zero_grad();
            loss->backward();
            test.optimizer->step();
            
            if (i % 10 == 0) {
                std::cout << "  Iter " << std::setw(2) << i 
                          << " | x = " << std::fixed << std::setprecision(4) 
                          << test.x->data()[0] 
                          << " | f(x) = " << loss->data()[0] << "\n";
            }
        }
        
        std::cout << "  Final: x = " << test.x->data()[0] 
                  << " (distance from 0: " << std::abs(test.x->data()[0]) << ")\n\n";
    }
}

// Test 6: DataLoader
void test_dataloader() {
    print_separator("Test 6: DataLoader و Batching");
    
    // Create dataset
    auto dataset = create_random_dataset(100, Shape{10}, Shape{1});
    std::cout << "Created dataset:\n";
    dataset.print_stats();
    
    // Create dataloader
    size_t batch_size = 16;
    DataLoader loader(dataset, batch_size, true);
    
    std::cout << "\nDataLoader:\n";
    std::cout << "  Batch size: " << batch_size << "\n";
    std::cout << "  Number of batches: " << loader.num_batches() << "\n";
    
    // Iterate through batches
    std::cout << "\nIterating through batches:\n";
    int batch_idx = 0;
    for (auto [batch_data, batch_labels] : loader) {
        std::cout << "  Batch " << batch_idx++ 
                  << ": " << batch_data.size() << " samples\n";
        
        if (batch_idx >= 3) break;  // Show first 3 batches only
    }
}

// Test 7: Computational Graph Visualization
void test_computational_graph() {
    print_separator("Test 7: Computational Graph");
    
    std::cout << "Building computational graph:\n\n";
    std::cout << "Graph structure:\n";
    std::cout << "  x ──┬──> x²\n";
    std::cout << "      │      │\n";
    std::cout << "      │      ├──> (x² + y)\n";
    std::cout << "  y ──┼──────┘      │\n";
    std::cout << "      │             │\n";
    std::cout << "      └──> xy ──────┤\n";
    std::cout << "                    │\n";
    std::cout << "                    v\n";
    std::cout << "              loss = (x² + y) + xy\n\n";
    
    auto x = std::make_shared<Tensor>(std::vector<double>{2.0}, Shape{1}, true);
    auto y = std::make_shared<Tensor>(std::vector<double>{3.0}, Shape{1}, true);
    
    x->set_name("x");
    y->set_name("y");
    
    // Build graph
    auto x_squared = x * x;
    x_squared->set_name("x²");
    
    auto xy = x * y;
    xy->set_name("xy");
    
    auto term1 = x_squared + y;
    term1->set_name("x²+y");
    
    auto loss = term1 + xy;
    loss->set_name("loss");
    
    std::cout << "Forward pass:\n";
    std::cout << "  x = " << x->data()[0] << "\n";
    std::cout << "  y = " << y->data()[0] << "\n";
    std::cout << "  x² = " << x_squared->data()[0] << "\n";
    std::cout << "  xy = " << xy->data()[0] << "\n";
    std::cout << "  x²+y = " << term1->data()[0] << "\n";
    std::cout << "  loss = " << loss->data()[0] << "\n";
    
    std::cout << "\nBackward pass:\n";
    loss->backward();
    
    std::cout << "  ∂loss/∂x = " << x->grad()[0] 
              << " (expected: 2x + y = " << 2*x->data()[0] + y->data()[0] << ")\n";
    std::cout << "  ∂loss/∂y = " << y->grad()[0] 
              << " (expected: 1 + x = " << 1 + x->data()[0] << ")\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                           ║\n";
    std::cout << "║     DARV Advanced Autograd Engine - Complete Test        ║\n";
    std::cout << "║           نظام Autograd المتقدم - اختبار شامل            ║\n";
    std::cout << "║                                                           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    
    try {
        test_tensor_operations();
        test_matmul();
        test_activations();
        test_neural_network();
        test_optimizers();
        test_dataloader();
        test_computational_graph();
        
        print_separator("✓ جميع الاختبارات اكتملت بنجاح!");
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}