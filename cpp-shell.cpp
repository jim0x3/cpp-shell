#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

const std::unordered_set<std::string> kBuiltins = {
    "echo", "exit", "type", "cd", "pwd",
    "history", "jobs", "complete", "declare",
};

std::vector<std::string> splitFields(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> fields;
    std::string field;
    while (iss >> field) {
        fields.push_back(field);
    }
    return fields;
}

std::string findExe(const std::string& exe) {
    const char* pathEnv = std::getenv("PATH");
    if (!pathEnv) {
        return "";
    }

    std::istringstream paths(pathEnv);
    std::string dir;
    while (std::getline(paths, dir, ':')) {
        fs::path candidate = fs::path(dir) / exe;
        std::error_code ec;
        auto status = fs::status(candidate, ec);
        if (ec || !fs::is_regular_file(status)) {
            continue;
        }

        struct stat st {};
        if (::stat(candidate.c_str(), &st) == 0 && (st.st_mode & 0111) != 0) {
            return candidate.string();
        }
    }
    return "";
}

int runExternal(const std::string& cmd, const std::vector<std::string>& args) {
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(cmd.c_str()));
    for (size_t i = 1; i < args.size(); ++i) {
        argv.push_back(const_cast<char*>(args[i].c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0) {
        std::perror("fork");
        return 1;
    }
    if (pid == 0) {
        execvp(cmd.c_str(), argv.data());
        _exit(127);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
        return 127;
    }
    return 0;
}

int main() {
    std::string line;
    while (true) {
        std::cout << "$ " << std::flush;
        if (!std::getline(std::cin, line)) {
            return 0;  // EOF
        }

        auto first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            continue;
        }
        auto last = line.find_last_not_of(" \t\r\n");
        line = line.substr(first, last - first + 1);

        auto args = splitFields(line);
        if (args.empty()) {
            continue;
        }

        const std::string& cmd = args[0];

        if (cmd == "exit") {
            int exitCode = 0;
            if (args.size() > 1) {
                try {
                    exitCode = std::stoi(args[1]);
                } catch (...) {
                }
            }
            return exitCode;
        }

        if (cmd == "echo") {
            for (size_t i = 1; i < args.size(); ++i) {
                if (i > 1) {
                    std::cout << ' ';
                }
                std::cout << args[i];
            }
            std::cout << '\n';
            continue;
        }

        if (cmd == "type") {
            if (args.size() < 2) {
                continue;
            }
            const std::string& target = args[1];
            if (kBuiltins.count(target)) {
                std::cout << target << " is a shell builtin\n";
                continue;
            }
            std::string fullPath = findExe(target);
            if (!fullPath.empty()) {
                std::cout << target << " is " << fullPath << '\n';
                continue;
            }
            std::cout << target << ": not found\n";
            continue;
        }

        if (cmd == "pwd") {
            std::error_code ec;
            auto pwd = fs::current_path(ec);
            if (ec) {
                std::cout << pwd.string() << ": not found\n";
            }
            std::cout << pwd.string() << '\n';
            continue;
        }

        if (cmd == "cd") {
            std::string path = args.size() > 1 ? args[1] : "";
            if (path == "~") {
                const char* home = std::getenv("HOME");
                path = home ? home : "";
            }
            if (::chdir(path.c_str()) != 0) {
                std::cout << "cd: /non-existing-directory: No such file or directory\n";
            }
            continue;
        }

        if (kBuiltins.count(cmd)) {
            continue;
        }

        int rc = runExternal(cmd, args);
        if (rc == 127) {
            std::cout << cmd << ": command not found\n";
        }
    }
}
