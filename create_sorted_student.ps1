# PowerShell script to create student_sorted.txt
# Sorts student.txt by roll number (second field)

Write-Host "Creating student_sorted.txt from student.txt..."

$inputFile = ".\data\student.txt"
$outputFile = ".\data\student_sorted.txt"

# Read all lines
$lines = Get-Content $inputFile

# Separate header and data
$header = $lines[0]
$dataLines = $lines[1..($lines.Count - 1)]

# Sort data lines by the second field (roll number) as string
$sortedData = $dataLines | Sort-Object {
    $fields = $_ -split ';'
    if ($fields.Count -gt 1) {
        $fields[1]
    } else {
        ""
    }
}

# Write output
$header | Out-File -FilePath $outputFile -Encoding ASCII
$sortedData | Out-File -FilePath $outputFile -Append -Encoding ASCII

Write-Host "Done! Created $outputFile with $($sortedData.Count) sorted records."
