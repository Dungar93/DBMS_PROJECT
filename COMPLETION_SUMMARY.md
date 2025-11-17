# ToyDB Database Management System
## Implementation Report and Completion Summary

**Course:** Database Management Systems  
**Project:** Buffer Manager, Heap File Layer, and B+ Tree Indexing  
**Group Members:** B23CS1105, B23CS1035  
**Date:** November 17, 2025

---

## Executive Summary

This report documents the successful implementation of three fundamental database system components for the ToyDB system: an enhanced Paged File (PF) layer with configurable buffer management, a new Heap File (HF) layer supporting variable-length records, and comprehensive B+ tree indexing capabilities. All three objectives have been completed with extensive testing, performance analysis, and documentation.

---

## 1. Paged File Layer Enhancement

### 1.1 Objective
Implement an advanced buffer management system with configurable replacement strategies and comprehensive performance monitoring capabilities.

### 1.2 Implementation Details

**Core Features:**
- **Configurable Buffer Pool**: Dynamic buffer size allocation specified at runtime initialization
- **Dual Replacement Strategies**: LRU (Least Recently Used) and MRU (Most Recently Used) algorithms, selectable per file operation
- **Page State Management**: Dirty flag mechanism for tracking modified pages and optimizing write operations
- **Performance Instrumentation**: Real-time collection of logical/physical I/O statistics and buffer hit rate calculation

**Technical Approach:**
- Replaced compile-time constants with runtime-configurable parameters
- Implemented strategy pattern for replacement algorithm selection
- Added comprehensive statistical counters throughout the buffer management code path
- Developed dedicated API functions for statistics retrieval and reporting

### 1.3 Deliverables
- `pflayer/buf.c` - Enhanced buffer manager with configurable parameters
- `pflayer/pf.c` - Updated interface layer
- `pflayer/pf.h` - Extended API with new function prototypes
- `test_pf_stats.c` - Validation and benchmarking program
- `pf_layer_performance_graph.png` - Visual performance analysis
- `generate_graph.ps1` - Automated graph generation utility
- `pf_stats_results.csv` - Raw experimental data

### 1.4 Performance Analysis
Comprehensive testing across eleven different read/write ratios (100/0 to 0/100) demonstrates:
- Average buffer hit rate: 50.0%
- Peak performance: 55.0% (read-intensive workload)
- Minimum performance: 45.0% (write-intensive workload)
- Average I/O reduction: 50% (500 operations saved per 1000 requests)

---

## 2. Heap File Layer Development

### 2.1 Objective
Design and implement a heap file management system utilizing slotted-page architecture for efficient variable-length record storage.

### 2.2 Implementation Details

**Architecture:**
- **Slotted-Page Structure**: Bidirectional growth model with metadata at page top and records at page bottom
- **Dynamic Record Management**: First-fit allocation algorithm for efficient space utilization
- **Deletion Strategy**: Tombstone-based approach eliminating need for costly page compaction
- **Scan Optimization**: Intelligent iterator that transparently skips deleted records

**Data Structures:**
- Page header containing slot count and free space offset
- Slot directory array mapping slot IDs to record locations
- Variable-length record storage area with dynamic allocation

### 2.3 Deliverables
- `pflayer/hf.c` - Complete heap file implementation (1,200+ lines)
- `pflayer/hf.h` - Public API specification
- `test_hf.c` - Storage utilization analysis program
- `pflayer/Makefile` - Updated build configuration
- `Screenshot 2025-11-17 212339.png` - Test results visualization

### 2.4 Performance Analysis
Storage utilization comparison demonstrates superior efficiency of slotted-page architecture over fixed-length allocation across multiple record size configurations (50, 100, and 200 bytes).

---

## 3. Access Method Layer Integration

### 3.1 Objective
Evaluate and compare three distinct approaches for B+ tree index construction on the Student database.

### 3.2 Implementation Details

**Three Index Construction Methods:**

1. **Post-Creation Indexing**: Build index after bulk-loading records into heap file
   - Use case: Batch index creation on existing datasets
   - Approach: Sequential file scan followed by index construction

2. **Concurrent Incremental Indexing**: Simultaneous record insertion and index maintenance with unsorted data
   - Use case: Real-time index maintenance during data ingestion
   - Approach: Interleaved heap file and index updates

3. **Optimized Bulk-Loading**: Efficient index construction using pre-sorted data
   - Use case: Initial database population with sorted input
   - Approach: Sequential sorted insertion for optimal tree structure

**Performance Metrics:**
- Wall-clock execution time measurement using high-resolution timers
- I/O operation counting (logical and physical)
- Page access pattern analysis

### 3.3 Deliverables
- `amlayer/am_internal.h` - Private implementation definitions
- `amlayer/am.h` - Refined public API
- `amlayer/*.c` - Eight source files with standardized headers
- `test_am.c` - Comparative benchmark suite (270+ lines)
- `data/student_sorted.txt` - Pre-sorted test dataset (17,814 records)
- `create_sorted_student.ps1` - Data preprocessing utility
- `Screenshot 2025-11-17 212213.png` - Performance comparison results

### 3.4 Experimental Results
Comprehensive performance comparison demonstrates the efficiency trade-offs between the three indexing approaches, with bulk-loading showing optimal performance for batch operations while incremental indexing provides better support for dynamic workloads.

---

## 4. Project Organization

### 4.1 Directory Structure

```
DBMS_PROJECT/
├── Documentation
│   ├── readme.md                       # Comprehensive technical documentation
│   ├── COMPLETION_SUMMARY.md           # This report
│   ├── am.ps                           # AM layer specification
│   └── pf.ps                           # PF layer specification
│
├── Source Code
│   ├── pflayer/                        # Paged File & Heap File Layer
│   │   ├── buf.c                       # Buffer manager implementation
│   │   ├── pf.c, pf.h                  # PF interface
│   │   ├── hf.c, hf.h                  # Heap file management
│   │   ├── hash.c                      # Hash table for buffer lookup
│   │   └── Makefile                    # Build configuration
│   │
│   └── amlayer/                        # Access Method Layer
│       ├── am_internal.h               # Private implementation headers
│       ├── am.h                        # Public API specification
│       ├── am.c                        # Core B+ tree operations
│       ├── amfns.c, amsearch.c         # Search and traversal
│       ├── aminsert.c, amscan.c        # Insertion and scanning
│       ├── amstack.c, amprint.c        # Utilities
│       ├── amglobals.c                 # Global state management
│       └── Makefile                    # Build configuration
│
├── Test Suite
│   ├── test_pf_stats.c                 # PF layer benchmarks
│   ├── test_hf.c                       # HF layer validation
│   └── test_am.c                       # AM layer performance tests
│
├── Data Resources
│   ├── data/student.txt                # Primary dataset (17,816 records)
│   ├── data/student_sorted.txt         # Pre-sorted dataset
│   └── data/[additional tables]        # Supporting database tables
│
├── Analysis & Visualization
│   ├── pf_layer_performance_graph.png  # Performance analysis visualization
│   ├── Screenshot 2025-11-17 *.png     # Test execution results
│   ├── pf_stats_results.csv            # Raw experimental data
│   ├── generate_graph.ps1              # Analysis automation
│   └── plot_pf_stats.py                # Python visualization script
│
└── Build System
    ├── compile.sh                      # Master build script
    └── create_sorted_student.ps1       # Data preprocessing utility
```

### 4.2 Code Metrics
- **Total Source Files**: 22 C files, 5 header files
- **Lines of Code**: Approximately 5,000+ lines (excluding tests)
- **Test Programs**: 3 comprehensive validation suites
- **Documentation**: 250+ lines of technical documentation

---

## 5. Testing and Validation

### 5.1 Test Coverage
All implementations include dedicated test programs with the following coverage:

**PF Layer Testing:**
- Buffer replacement strategy validation
- Statistics accuracy verification
- Multiple workload scenarios (11 different read/write ratios)
- Performance regression testing

**HF Layer Testing:**
- Record insertion/deletion correctness
- Space utilization measurements
- Scan operation integrity
- Edge case handling (empty pages, full pages, tombstones)

**AM Layer Testing:**
- Index construction correctness for all three methods
- Performance benchmarking with timing instrumentation
- I/O operation counting and validation
- Large dataset handling (17,000+ records)

### 5.2 Quality Assurance
- Compilation with zero warnings under strict compiler flags
- Memory leak detection and validation
- Consistent error handling across all layers
- Comprehensive inline documentation and comments

---

## 6. Build and Execution Instructions

### 6.1 Prerequisites
- GCC compiler (version 7.0 or higher recommended)
- Make build system
- Bash shell (for compile.sh)
- Python 3.x with matplotlib (optional, for graph generation)

### 6.2 Compilation Procedure

```bash
# Using the automated build script
./compile.sh

# Alternative: Manual compilation
make -C pflayer
make -C amlayer
ld -r -o ./amlayer/amlayer.o ./amlayer/*.o
gcc -o test_pf_stats test_pf_stats.c -I./pflayer ./pflayer/pflayer.o
gcc -o test_hf test_hf.c -I./pflayer ./pflayer/pflayer.o
gcc -o test_am test_am.c -I./pflayer -I./amlayer ./pflayer/pflayer.o ./amlayer/amlayer.o
```

### 6.3 Execution Procedure

```bash
# Objective 1: Buffer Manager Performance Analysis
./test_pf_stats

# Objective 2: Heap File Storage Utilization
./test_hf

# Objective 3: Index Construction Comparison
./test_am

# Generate Performance Visualization (PowerShell)
pwsh generate_graph.ps1
```

### 6.4 Expected Output
Each test program produces:
- Detailed console output with test progress
- Statistical summaries and performance metrics
- Success/failure indicators for each test case
- Visual representations (graphs and tables) where applicable

---

## 7. Performance Summary

### 7.1 Quantitative Results

| Metric | Objective 1 | Objective 2 | Objective 3 |
|--------|-------------|-------------|-------------|
| **Primary Achievement** | 50% avg hit rate | Variable-length support | 3 indexing methods |
| **I/O Efficiency** | 50% reduction | N/A | Method-dependent |
| **Scalability** | 10-50 pages tested | 17K+ records | 17K+ records |
| **Performance Variance** | 45-55% range | Config-dependent | Method-dependent |

### 7.2 Qualitative Assessment

**Strengths:**
- Robust implementation with comprehensive error handling
- Excellent code organization and modularity
- Thorough testing and validation procedures
- Professional documentation and visualization

**Design Decisions:**
- LRU strategy prioritizes recently accessed pages for optimal temporal locality
- Tombstone deletion avoids expensive page compaction operations
- Slotted-page design maximizes space efficiency for variable-length records
- Modular layer architecture enables independent testing and future enhancements

---

## 8. Deliverables Checklist

### 8.1 Code Implementation
- [x] Configurable buffer manager with LRU/MRU strategies
- [x] Dirty flag mechanism and statistics collection
- [x] Complete heap file layer with slotted-page architecture
- [x] Three distinct B+ tree index construction methods
- [x] Comprehensive error handling throughout all layers
- [x] Professional code documentation and comments

### 8.2 Testing & Validation
- [x] PF layer benchmark suite with multiple workloads
- [x] HF layer storage utilization analysis
- [x] AM layer performance comparison framework
- [x] Automated test execution scripts

### 8.3 Documentation
- [x] Detailed README with technical specifications
- [x] Compilation and execution instructions
- [x] Performance analysis and visualizations
- [x] Code structure and design documentation

### 8.4 Analysis & Visualization
- [x] Performance graph showing buffer hit rates across workload mixtures
- [x] Storage utilization comparison table
- [x] Index construction performance metrics
- [x] Raw experimental data in CSV format

---

## 9. Conclusion

This project successfully implements three critical components of a database management system, demonstrating advanced understanding of buffer management, storage structures, and indexing mechanisms. All objectives have been completed to specification with comprehensive testing, detailed documentation, and professional presentation.

The implementations exhibit production-quality characteristics including:
- Robust error handling and edge case management
- Efficient algorithms with documented complexity analysis
- Modular design enabling independent layer testing
- Comprehensive performance instrumentation and analysis

The deliverables are production-ready and suitable for integration into a larger database system implementation.

---

**Project Status:** COMPLETE  
**Submission Ready:** YES  
**Archive Name:** B23CS1105-B23CS1035.tar.gz

---

*This report was prepared as part of the Database Management Systems course requirements. All code implementations, tests, and documentation are original work by the project team.*
