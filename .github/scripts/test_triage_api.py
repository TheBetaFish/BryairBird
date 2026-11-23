#!/usr/bin/env python3
"""Unit tests for the API triage script."""

import os
import re
import subprocess
import sys
import tempfile
import unittest
from datetime import datetime

# Add the script directory to path so we can import triage_api
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import triage_api


class TestTriageApi(unittest.TestCase):
    def setUp(self):
        # Create a temporary git repository for testing
        self.temp_dir = tempfile.TemporaryDirectory()
        self.repo_dir = self.temp_dir.name
        
        # Initialize git repo
        subprocess.run(["git", "init"], cwd=self.repo_dir, check=True, capture_output=True)
        subprocess.run(["git", "config", "user.name", "Test User"], cwd=self.repo_dir, check=True, capture_output=True)
        subprocess.run(["git", "config", "user.email", "test@example.com"], cwd=self.repo_dir, check=True, capture_output=True)
        
        # Create a basic structure
        os.makedirs(os.path.join(self.repo_dir, "native/guest"), exist_ok=True)
        os.makedirs(os.path.join(self.repo_dir, "native"), exist_ok=True)
        
        # Create a dummy registry file
        registry_content = '''
namespace raven {
void register_api(ApiRegistry &registry, KernelContext &context) {
    registry.add("CreateFileW", (void*)create_file_w);
    registry.add("CloseHandle", (void*)close_handle);
}
}'''
        with open(os.path.join(self.repo_dir, "native", "dummy_registry.cpp"), "w") as f:
            f.write(registry_content)
            
        # Create a dummy guest DLL def file
        def_content = '''
EXPORTS
    CreateFileW
    CloseHandle
    WriteFile
'''
        with open(os.path.join(self.repo_dir, "native/guest", "dummy.dll.def"), "w") as f:
            f.write(def_content)
            
        # Create a dummy exports.txt file
        exports_txt_content = '''
WriteFile
ReadFile
'''
        with open(os.path.join(self.repo_dir, "native/guest", "dummy.exports.txt"), "w") as f:
            f.write(exports_txt_content)
            
        # Create a dummy source file with implementations
        source_content = '''
#include <windows.h>

BOOL WINAPI create_file_w(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                          LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                          DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    return TRUE;
}

BOOL WINAPI close_handle(HANDLE hObject) {
    return TRUE;
}

BOOL WINAPI write_file(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                       LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    return TRUE;
}
'''
        with open(os.path.join(self.repo_dir, "native/guest", "dummy_source.c"), "w") as f:
            f.write(source_content)
            
        # Create a dummy source file with dllexport
        dllexport_content = '''
#include <windows.h>

__declspec(dllexport) BOOL WINAPI read_file(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                                           LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    return TRUE;
}
'''
        with open(os.path.join(self.repo_dir, "native/guest", "dllexport_source.c"), "w") as f:
            f.write(dllexport_content)
            
        # Commit everything
        subprocess.run(["git", "add", "."], cwd=self.repo_dir, check=True, capture_output=True)
        subprocess.run(["git", "commit", "-m", "Initial commit"], cwd=self.repo_dir, check=True, capture_output=True)
        
        # Create a fake commit for testing
        subprocess.run(["git", "commit", "--allow-empty", "-m", "Fake commit for testing"], cwd=self.repo_dir, check=True, capture_output=True)
        
        # Mock git function to use our test repo
        self.original_git = triage_api.git
        
    def tearDown(self):
        self.temp_dir.cleanup()
        
    def mock_git(self, *arguments):
        """Mock git function that uses our test repository."""
        if arguments[0] == "grep":
            # Simulate grep results for our test files
            if "-l" in arguments and "-F" in arguments and "registry.add" in arguments:
                return os.path.join(self.repo_dir, "native", "dummy_registry.cpp") + "\n"
            elif "-l" in arguments and "-w" in arguments and "-E" in arguments:
                # Check for exports
                if "WriteFile" in arguments or "ReadFile" in arguments:
                    return os.path.join(self.repo_dir, "native/guest", "dummy.exports.txt") + "\n"
                elif "CreateFileW" in arguments or "CloseHandle" in arguments:
                    return os.path.join(self.repo_dir, "native/guest", "dummy.dll.def") + "\n"
            elif "-l" in arguments and "-E" in arguments:
                # Check for implementations
                if "create_file_w" in arguments or "close_handle" in arguments or "write_file" in arguments:
                    return os.path.join(self.repo_dir, "native/guest", "dummy_source.c") + "\n"
                elif "read_file" in arguments:
                    return os.path.join(self.repo_dir, "native/guest", "dllexport_source.c") + "\n"
        elif arguments[0] == "log":
            # Return a fake commit
            if "-S" in arguments and "--" in arguments:
                return "12345678 12345678 2023-01-01\n"
        elif arguments[0] == "rev-parse":
            return self.repo_dir + "\n"
        # For other cases, fall back to original
        return self.original_git(*arguments)
        
    def test_answer_with_registry_handler(self):
        """Test that a function registered with registry.add is correctly identified."""
        # Mock git function
        original_git = triage_api.git
        triage_api.git = self.mock_git
        
        try:
            result = triage_api.answer("CreateFileW")
            self.assertIsNotNone(result)
            self.assertEqual(result[0], os.path.join(self.repo_dir, "native", "dummy_registry.cpp"))
        finally:
            triage_api.git = original_git
            
    def test_answer_with_implementation_and_export(self):
        """Test that a function with both implementation and export returns the implementation."""
        # Mock git function
        original_git = triage_api.git
        triage_api.git = self.mock_git
        
        try:
            result = triage_api.answer("write_file")
            self.assertIsNotNone(result)
            # Should return implementation file, not export file
            self.assertEqual(result[0], os.path.join(self.repo_dir, "native/guest", "dummy_source.c"))
        finally:
            triage_api.git = original_git
            
    def test_answer_with_only_export(self):
        """Test that a function only in export table is NOT counted as implemented."""
        # Mock git function
        original_git = triage_api.git
        triage_api.git = self.mock_git
        
        try:
            result = triage_api.answer("WriteFile")
            self.assertIsNone(result)
        finally:
            triage_api.git = original_git
            
    def test_answer_with_dllexport(self):
        """Test that a function with dllexport is correctly identified."""
        # Mock git function
        original_git = triage_api.git
        triage_api.git = self.mock_git
        
        try:
            result = triage_api.answer("read_file")
            self.assertIsNotNone(result)
            self.assertEqual(result[0], os.path.join(self.repo_dir, "native/guest", "dllexport_source.c"))
        finally:
            triage_api.git = original_git
            
    def test_answer_nonexistent_function(self):
        """Test that a non-existent function returns None."""
        # Mock git function
        original_git = triage_api.git
        triage_api.git = self.mock_git
        
        try:
            result = triage_api.answer("NonExistentFunction")
            self.assertIsNone(result)
        finally:
            triage_api.git = original_git


if __name__ == "__main__":
    unittest.main()
