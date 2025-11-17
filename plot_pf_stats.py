import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

# Read the CSV data
df = pd.read_csv('pf_stats_results.csv')

# Create figure with subplots
fig = plt.figure(figsize=(16, 10))
gs = fig.add_gridspec(3, 2, hspace=0.3, wspace=0.3)

fig.suptitle('PF Layer Buffer Performance Analysis with LRU Strategy\n(Buffer Size: 10 pages, Total Pages: 50, Requests: 1000)', 
             fontsize=16, fontweight='bold')

# Create read/write ratio labels
labels = [f"{r}/{w}" for r, w in zip(df['ReadPercent'], df['WritePercent'])]
x = np.arange(len(labels))

# Plot 1: Logical vs Physical Reads
ax1 = fig.add_subplot(gs[0, 0])
width = 0.35
ax1.bar(x - width/2, df['LogicalReads'], width, label='Logical Reads', color='#3498db', alpha=0.9)
ax1.bar(x + width/2, df['PhysicalReads'], width, label='Physical Reads', color='#e74c3c', alpha=0.9)
ax1.set_xlabel('Read/Write Ratio (%)', fontweight='bold', fontsize=11)
ax1.set_ylabel('Number of Operations', fontweight='bold', fontsize=11)
ax1.set_title('Read Operations: Logical vs Physical', fontweight='bold', fontsize=12)
ax1.set_xticks(x)
ax1.set_xticklabels(labels, rotation=45, ha='right', fontsize=9)
ax1.legend(fontsize=10)
ax1.grid(axis='y', alpha=0.3, linestyle='--')

# Plot 2: Logical vs Physical Writes
ax2 = fig.add_subplot(gs[0, 1])
ax2.bar(x - width/2, df['LogicalWrites'], width, label='Logical Writes', color='#2ecc71', alpha=0.9)
ax2.bar(x + width/2, df['PhysicalWrites'], width, label='Physical Writes', color='#f39c12', alpha=0.9)
ax2.set_xlabel('Read/Write Ratio (%)', fontweight='bold', fontsize=11)
ax2.set_ylabel('Number of Operations', fontweight='bold', fontsize=11)
ax2.set_title('Write Operations: Logical vs Physical', fontweight='bold', fontsize=12)
ax2.set_xticks(x)
ax2.set_xticklabels(labels, rotation=45, ha='right', fontsize=9)
ax2.legend(fontsize=10)
ax2.grid(axis='y', alpha=0.3, linestyle='--')

# Plot 3: Buffer Hit Rate
ax3 = fig.add_subplot(gs[1, :])
line = ax3.plot(x, df['HitRate'], marker='o', linewidth=3, markersize=10, 
                color='#9b59b6', label='Buffer Hit Rate')
ax3.fill_between(x, df['HitRate'], alpha=0.3, color='#9b59b6')
ax3.set_xlabel('Read/Write Ratio (%)', fontweight='bold', fontsize=12)
ax3.set_ylabel('Hit Rate (%)', fontweight='bold', fontsize=12)
ax3.set_title('Buffer Hit Rate Across Different Workload Mixtures', fontweight='bold', fontsize=13)
ax3.set_xticks(x)
ax3.set_xticklabels(labels, rotation=45, ha='right', fontsize=10)
ax3.grid(True, alpha=0.3, linestyle='--')
ax3.set_ylim([0, 100])
ax3.axhline(y=50, color='red', linestyle='--', alpha=0.5, label='50% Hit Rate')
ax3.legend(fontsize=11)

# Add value labels on the line
for i, (xi, yi) in enumerate(zip(x, df['HitRate'])):
    ax3.annotate(f'{yi:.1f}%', (xi, yi), textcoords="offset points", 
                xytext=(0,10), ha='center', fontsize=8, fontweight='bold')

# Plot 4: Total I/O Comparison
ax4 = fig.add_subplot(gs[2, 0])
ax4.plot(x, df['TotalLogicalIO'], marker='s', linewidth=2.5, markersize=8, 
         label='Total Logical I/O', color='#3498db', linestyle='-')
ax4.plot(x, df['TotalPhysicalIO'], marker='^', linewidth=2.5, markersize=8, 
         label='Total Physical I/O', color='#e74c3c', linestyle='-')
ax4.set_xlabel('Read/Write Ratio (%)', fontweight='bold', fontsize=11)
ax4.set_ylabel('Number of Operations', fontweight='bold', fontsize=11)
ax4.set_title('Total I/O Operations Comparison', fontweight='bold', fontsize=12)
ax4.set_xticks(x)
ax4.set_xticklabels(labels, rotation=45, ha='right', fontsize=9)
ax4.legend(fontsize=10)
ax4.grid(True, alpha=0.3, linestyle='--')

# Plot 5: I/O Efficiency (Physical/Logical ratio)
ax5 = fig.add_subplot(gs[2, 1])
efficiency = (df['TotalPhysicalIO'] / df['TotalLogicalIO'] * 100)
colors = ['#27ae60' if e < 50 else '#e67e22' if e < 60 else '#e74c3c' for e in efficiency]
bars = ax5.bar(x, efficiency, color=colors, alpha=0.8, edgecolor='black', linewidth=1.5)
ax5.set_xlabel('Read/Write Ratio (%)', fontweight='bold', fontsize=11)
ax5.set_ylabel('Physical I/O Ratio (%)', fontweight='bold', fontsize=11)
ax5.set_title('I/O Efficiency (Lower is Better)', fontweight='bold', fontsize=12)
ax5.set_xticks(x)
ax5.set_xticklabels(labels, rotation=45, ha='right', fontsize=9)
ax5.grid(axis='y', alpha=0.3, linestyle='--')
ax5.axhline(y=50, color='red', linestyle='--', alpha=0.7, linewidth=2)

# Add value labels on bars
for i, (bar, val) in enumerate(zip(bars, efficiency)):
    height = bar.get_height()
    ax5.text(bar.get_x() + bar.get_width()/2., height + 1,
            f'{val:.1f}%', ha='center', va='bottom', fontsize=8, fontweight='bold')

plt.savefig('pf_layer_performance_graph.png', dpi=300, bbox_inches='tight', facecolor='white')
print("\n" + "="*60)
print("Graph saved as: pf_layer_performance_graph.png")
print("="*60)

# Print summary statistics
print("\n=== Summary Statistics ===")
print(f"Average Buffer Hit Rate: {df['HitRate'].mean():.2f}%")
print(f"Best Hit Rate: {df['HitRate'].max():.2f}% (at {labels[df['HitRate'].idxmax()]})")
print(f"Worst Hit Rate: {df['HitRate'].min():.2f}% (at {labels[df['HitRate'].idxmin()]})")
print(f"\nPhysical Reads Range: {df['PhysicalReads'].min()} - {df['PhysicalReads'].max()}")
print(f"Physical Writes Range: {df['PhysicalWrites'].min()} - {df['PhysicalWrites'].max()}")
print(f"\nTotal Logical I/O: {df['TotalLogicalIO'].iloc[0]} (constant across all workloads)")
print(f"Physical I/O Range: {df['TotalPhysicalIO'].min()} - {df['TotalPhysicalIO'].max()}")
print(f"\nBuffer saves approx. {1000 - df['TotalPhysicalIO'].mean():.0f} I/O operations on average")
print("="*60 + "\n")
