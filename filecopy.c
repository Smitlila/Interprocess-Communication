#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

#define READ_END 0
#define WRITE_END 1

int main(int argc, char *argv[])
{
    // Check if the correct number of command-line arguments are provided
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_file> <destination_file>\n", argv[0]);
        return 1; // Return an error code
    }

    // Open the source file for reading
    int src_fd = open(argv[1], O_RDONLY);
    if (src_fd == -1) {
        perror("Error opening source file");
        return 1; // Return an error code
    }

    // Open the destination file for writing
    int dest_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (dest_fd == -1) {
        fprintf(stderr, "Unable to open destination file \"%s\"\n", argv[2]);
        perror(""); // Print system error message
        close(src_fd); // Close the source file descriptor
        return 1; // Return an error code
    }

    // Create a pipe
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("Pipe creation failed");
        close(src_fd); // Close the source file descriptor
        close(dest_fd); // Close the destination file descriptor
        return 1; // Return an error code
    }

    // Fork a child process
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        close(src_fd); // Close the source file descriptor
        close(dest_fd); // Close the destination file descriptor
        close(pipefd[READ_END]); // Close the read end of the pipe
        close(pipefd[WRITE_END]); // Close the write end of the pipe
        return 1; // Return an error code
    }

    if (pid > 0) { // Parent process
        // Close unnecessary pipe ends
        close(pipefd[READ_END]);

        // Copy data from source file to pipe
        char buffer[BUFSIZ];
        ssize_t bytes_read;
        while ((bytes_read = read(src_fd, buffer, BUFSIZ)) > 0) {
            write(pipefd[WRITE_END], buffer, bytes_read);
        }

        // Close remaining file descriptors
        close(src_fd);
        close(pipefd[WRITE_END]);

        // Wait for the child process to finish
        wait(NULL);

        // Print success message
        fprintf(stderr, "File successfully copied from %s to %s\n", argv[1], argv[2]);
    } else { // Child process
        // Close unnecessary file descriptors
        close(src_fd);
        close(pipefd[WRITE_END]);

        // Copy data from pipe to destination file
        char buffer[BUFSIZ];
        ssize_t bytes_read;
        while ((bytes_read = read(pipefd[READ_END], buffer, BUFSIZ)) > 0) {
            write(dest_fd, buffer, bytes_read);
        }

        // Close remaining file descriptors
        close(dest_fd);
        close(pipefd[READ_END]);

        // Exit the child process
        exit(0);
    }

    return 0; // Exit successfully
}
