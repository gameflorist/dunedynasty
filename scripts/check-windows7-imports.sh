#!/bin/bash -e
#
# Fail if the packaged executable or DLLs need something Windows 7 does not
# have: functions added in Windows 8 or later, or the Universal C Runtime
# (not installed on Windows 7 by default). Imports looked up at run time
# (GetProcAddress) are not seen, and are fine.
#
# Usage: scripts/check-windows7-imports.sh <dist dir>

DIST="${1:-dist}"

# Functions added in Windows 8 or later that libraries commonly import.
NEWER='GetSystemTimePreciseAsFileTime|CreateFile2|CopyFile2|GetCurrentThreadStackLimits|SetThreadInformation|GetThreadInformation|GetProcessMitigationPolicy|SetProcessMitigationPolicy|PrefetchVirtualMemory|GetOverlappedResultEx|AddDllDirectory|RemoveDllDirectory|SetDefaultDllDirectories|GetFirmwareType|GetCurrentPackageId|GetPackageFullName|CreateFileMappingFromApp|MapViewOfFileFromApp|WaitOnAddress|WakeByAddressSingle|WakeByAddressAll|SetThreadDescription|GetThreadDescription|GetSystemCpuSetInformation|VirtualAlloc2|InitializeSynchronizationBarrier|EnterSynchronizationBarrier|GetPointerInfo|GetPointerType|EnableMouseInPointer|GetDpiForWindow|GetDpiForSystem|SetProcessDpiAwarenessContext|SetThreadDpiAwarenessContext|AdjustWindowRectExForDpi|SystemParametersInfoForDpi|GetSystemMetricsForDpi|EnableNonClientDpiScaling'

status=0
for f in "$DIST"/*.exe "$DIST"/*.dll; do
	imports=$(objdump -p "$f")
	if grep -qiE 'DLL Name: (api-ms-win-crt|ucrtbase)' <<<"$imports"; then
		echo "$(basename "$f"): needs the Universal C Runtime"
		status=1
	fi
	found=$(grep -oE "\b($NEWER)\b" <<<"$imports" | sort -u | tr '\n' ' ')
	if [ -n "$found" ]; then
		echo "$(basename "$f"): imports Windows 8+ functions: $found"
		status=1
	fi
done

[ $status = 0 ] && echo "No Windows 8+ imports in $DIST"
exit $status
