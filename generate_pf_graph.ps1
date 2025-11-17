# PowerShell script to generate PF layer performance graph
# This script runs test_pf_stats with different read/write ratios and collects statistics

Write-Host "=== Generating PF Layer Performance Graph Data ===" -ForegroundColor Green
Write-Host ""

# Create a modified test program that accepts read percentage as command line argument
$testCode = @'
/*
 * test_pf_stats_variable.c
 * Modified version to accept read percentage as argument
 */

#include <stdio.h>
#include <stdlib.h>
#include "pf.h"

#define FILE_NAME "test_stats.dat"
#define NUM_PAGES 50
#define NUM_REQUESTS 1000
#define BUF_SIZE 10

int main(int argc, char *argv[]) {
    int fd;
    int pagenum;
    char *pagebuf;
    int i, j;
    int read_percentage = 90; // Default
    
    if (argc > 1) {
        read_percentage = atoi(argv[1]);
        if (read_percentage < 0) read_percentage = 0;
        if (read_percentage > 100) read_percentage = 100;
    }
    
    // Initialize with LRU strategy
    PF_Init(BUF_SIZE, 0);
    
    // Create and populate test file
    PF_CreateFile(FILE_NAME);
    fd = PF_OpenFile(FILE_NAME);
    
    for (i = 0; i < NUM_PAGES; i++) {
        PF_AllocPage(fd, &pagenum, &pagebuf);
        *((int*)pagebuf) = i;
        PF_UnfixPage(fd, pagenum, 1);
    }
    
    // Run workload
    srand(42);
    for (i = 0; i < NUM_REQUESTS; i++) {
        pagenum = rand() % NUM_PAGES;
        PF_GetThisPage(fd, pagenum, &pagebuf);
        
        int is_write = (rand() % 100) >= read_percentage;
        if (is_write) {
            *((int*)pagebuf) = i;
            PF_UnfixPage(fd, pagenum, 1);
        } else {
            PF_UnfixPage(fd, pagenum, 0);
        }
    }
    
    // Print stats in parseable format
    printf("READ_WRITE_RATIO,%d/%d\n", read_percentage, 100-read_percentage);
    PF_PrintStats();
    
    PF_CloseFile(fd);
    PF_DestroyFile(FILE_NAME);
    
    return 0;
}
'@

# Write the modified test program
$testCode | Out-File -FilePath "test_pf_stats_variable.c" -Encoding ASCII

Write-Host "Compiling test_pf_stats_variable.c..." -ForegroundColor Cyan
& cc -o test_pf_stats_variable test_pf_stats_variable.c -I./pflayer ./pflayer/pflayer.o 2>&1 | Out-Null

if ($LASTEXITCODE -ne 0) {
    Write-Host "Compilation failed. Make sure pflayer is built." -ForegroundColor Red
    exit 1
}

# Test different read/write ratios
$ratios = @(100, 90, 80, 70, 60, 50, 40, 30, 20, 10, 0)
$results = @()

Write-Host ""
Write-Host "Running tests with different read/write ratios..." -ForegroundColor Cyan
Write-Host ""

foreach ($readPercent in $ratios) {
    $writePercent = 100 - $readPercent
    Write-Host "Testing ${readPercent}% Read / ${writePercent}% Write..." -ForegroundColor Yellow
    
    $output = & .\test_pf_stats_variable.exe $readPercent 2>&1 | Out-String
    
    # Parse the output
    $logicalReads = 0
    $physicalReads = 0
    $logicalWrites = 0
    $physicalWrites = 0
    $hitRate = 0.0
    
    if ($output -match 'Logical Reads:\s+(\d+)') { $logicalReads = [int]$Matches[1] }
    if ($output -match 'Physical Reads:\s+(\d+)') { $physicalReads = [int]$Matches[1] }
    if ($output -match 'Logical Writes:\s+(\d+)') { $logicalWrites = [int]$Matches[1] }
    if ($output -match 'Physical Writes:\s+(\d+)') { $physicalWrites = [int]$Matches[1] }
    if ($output -match 'Buffer Hit Rate:\s+([\d.]+)%') { $hitRate = [double]$Matches[1] }
    
    $results += [PSCustomObject]@{
        ReadPercent = $readPercent
        WritePercent = $writePercent
        LogicalReads = $logicalReads
        PhysicalReads = $physicalReads
        LogicalWrites = $logicalWrites
        PhysicalWrites = $physicalWrites
        HitRate = $hitRate
        TotalLogicalIO = $logicalReads + $logicalWrites
        TotalPhysicalIO = $physicalReads + $physicalWrites
    }
}

Write-Host ""
Write-Host "=== Test Results ===" -ForegroundColor Green
$results | Format-Table -AutoSize

# Export to CSV for graphing
$csvFile = "pf_stats_results.csv"
$results | Export-Csv -Path $csvFile -NoTypeInformation
Write-Host ""
Write-Host "Results saved to: $csvFile" -ForegroundColor Green

# Create Python script to generate the graph
$pythonScript = @'
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

# Read the CSV data
df = pd.read_csv('pf_stats_results.csv')

# Create figure with subplots
fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(14, 10))
fig.suptitle('PF Layer Buffer Performance Analysis\n(Buffer Size: 10, Total Pages: 50, Requests: 1000)', 
             fontsize=14, fontweight='bold')

# Create read/write ratio labels
labels = [f"{r}/{w}" for r, w in zip(df['ReadPercent'], df['WritePercent'])]

# Plot 1: Logical vs Physical I/O
x = np.arange(len(labels))
width = 0.35
ax1.bar(x - width/2, df['LogicalReads'], width, label='Logical Reads', color='skyblue', alpha=0.8)
ax1.bar(x + width/2, df['PhysicalReads'], width, label='Physical Reads', color='navy', alpha=0.8)
ax1.set_xlabel('Read/Write Ratio (%)', fontweight='bold')
ax1.set_ylabel('Number of Operations', fontweight='bold')
ax1.set_title('Logical vs Physical Reads', fontweight='bold')
ax1.set_xticks(x)
ax1.set_xticklabels(labels, rotation=45, ha='right')
ax1.legend()
ax1.grid(axis='y', alpha=0.3)

# Plot 2: Write Operations
ax2.bar(x - width/2, df['LogicalWrites'], width, label='Logical Writes', color='lightcoral', alpha=0.8)
ax2.bar(x + width/2, df['PhysicalWrites'], width, label='Physical Writes', color='darkred', alpha=0.8)
ax2.set_xlabel('Read/Write Ratio (%)', fontweight='bold')
ax2.set_ylabel('Number of Operations', fontweight='bold')
ax2.set_title('Logical vs Physical Writes', fontweight='bold')
ax2.set_xticks(x)
ax2.set_xticklabels(labels, rotation=45, ha='right')
ax2.legend()
ax2.grid(axis='y', alpha=0.3)

# Plot 3: Buffer Hit Rate
ax3.plot(labels, df['HitRate'], marker='o', linewidth=2, markersize=8, color='green')
ax3.fill_between(range(len(labels)), df['HitRate'], alpha=0.3, color='green')
ax3.set_xlabel('Read/Write Ratio (%)', fontweight='bold')
ax3.set_ylabel('Hit Rate (%)', fontweight='bold')
ax3.set_title('Buffer Hit Rate vs Read/Write Ratio', fontweight='bold')
ax3.set_xticks(range(len(labels)))
ax3.set_xticklabels(labels, rotation=45, ha='right')
ax3.grid(True, alpha=0.3)
ax3.set_ylim([0, 100])

# Plot 4: Total I/O Operations
ax4.plot(labels, df['TotalLogicalIO'], marker='s', linewidth=2, markersize=8, 
         label='Total Logical I/O', color='blue')
ax4.plot(labels, df['TotalPhysicalIO'], marker='^', linewidth=2, markersize=8, 
         label='Total Physical I/O', color='red')
ax4.set_xlabel('Read/Write Ratio (%)', fontweight='bold')
ax4.set_ylabel('Number of Operations', fontweight='bold')
ax4.set_title('Total I/O Operations', fontweight='bold')
ax4.set_xticks(range(len(labels)))
ax4.set_xticklabels(labels, rotation=45, ha='right')
ax4.legend()
ax4.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('pf_layer_performance_graph.png', dpi=300, bbox_inches='tight')
print("\nGraph saved as: pf_layer_performance_graph.png")
plt.show()

# Print summary statistics
print("\n=== Summary Statistics ===")
print(f"Average Buffer Hit Rate: {df['HitRate'].mean():.2f}%")
print(f"Best Hit Rate: {df['HitRate'].max():.2f}% (at {labels[df['HitRate'].idxmax()]})")
print(f"Worst Hit Rate: {df['HitRate'].min():.2f}% (at {labels[df['HitRate'].idxmin()]})")
print(f"\nTotal Physical Reads Range: {df['PhysicalReads'].min()} - {df['PhysicalReads'].max()}")
print(f"Total Physical Writes Range: {df['PhysicalWrites'].min()} - {df['PhysicalWrites'].max()}")
'@

$pythonScript | Out-File -FilePath "plot_pf_stats.py" -Encoding ASCII

Write-Host ""
Write-Host "=== Generating Graph ===" -ForegroundColor Green
Write-Host "Checking for Python and matplotlib..." -ForegroundColor Cyan

# Check if Python is available
$pythonCmd = $null
foreach ($cmd in @("python", "python3", "py")) {
    try {
        $version = & $cmd --version 2>&1
        if ($LASTEXITCODE -eq 0) {
            $pythonCmd = $cmd
            Write-Host "Found Python: $version" -ForegroundColor Green
            break
        }
    } catch {
        continue
    }
}

if ($pythonCmd) {
    Write-Host "Installing/checking matplotlib..." -ForegroundColor Cyan
    & $pythonCmd -m pip install matplotlib pandas --quiet 2>&1 | Out-Null
    
    Write-Host "Generating graph..." -ForegroundColor Cyan
    & $pythonCmd plot_pf_stats.py
    
    if (Test-Path "pf_layer_performance_graph.png") {
        Write-Host ""
        Write-Host "SUCCESS! Graph generated: pf_layer_performance_graph.png" -ForegroundColor Green
        Write-Host ""
        Write-Host "Opening graph..." -ForegroundColor Cyan
        Start-Process "pf_layer_performance_graph.png"
    }
} else {
    Write-Host ""
    Write-Host "Python not found. Please install Python and matplotlib to generate the graph." -ForegroundColor Yellow
    Write-Host "Data has been saved to: $csvFile" -ForegroundColor Yellow
    Write-Host "You can use Excel or any other tool to create the graph manually." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== Done ===" -ForegroundColor Green
