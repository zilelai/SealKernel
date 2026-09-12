#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char input [128];

void allpackage(){
    char command[600];
    snprintf(command, sizeof(command), "git clone https://github.com/zilelai/SealKernel");
            
    int status = system(command);
    if (status != 0) {
        printf("errcode 21: git clone failed\n");
    }

    system("cd SealKernel && g++ tty1.cpp -o tty1");
    system("cd SealKernel && g++ tty2.cpp -o tty2");
    system("cd SealKernel && gcc bootloader.c -o bootloader");

    printf("SealKernel directories: \n");
    printf("tty1 - /SealKernel/tty1\n");
    printf("tty2 - /SealKernel/tty2\n");
    printf("bootloader - /SealKernel/bootloader\n");
}

void tty1(){
    char command[600];
    snprintf(command, sizeof(command), "git clone https://github.com/zilelai/SealKernel");
            
    int status = system(command);
    if (status != 0) {
        printf("errcode 21: git clone failed\n");
    }

    system("cd SealKernel && rm tty2.cpp");
    system("cd SealKernel && rm bootloader.c");

    system("cd SealKernel && g++ tty1.cpp -o tty1");

    printf("SealKernel directories: \n");
    printf("tty1 - /SealKernel/tty1\n");
}

void tty2(){
    char command[600];
    snprintf(command, sizeof(command), "git clone https://github.com/zilelai/SealKernel");
            
    int status = system(command);
    if (status != 0) {
        printf("errcode 21: git clone failed\n");
    }

    system("cd SealKernel && rm tty1.cpp");
    system("cd SealKernel && rm bootloader.c");

    system("cd SealKernel && g++ tty2.cpp -o tty2");

    printf("SealKernel directories: \n");
    printf("tty2 - /SealKernel/tty2\n");
}

void sysfetch(){
    char command[600];
    snprintf(command, sizeof(command), "git clone https://github.com/zilelai/sysfetch");
            
    int status = system(command);
    if (status != 0) {
        printf("errcode 21: git clone failed\n");
    }

    system("cd sysfetch && gcc sysfetch.c -o sysfetch");

    printf("SealKernel directories: \n");
    printf("sysfetch - /sysfetch/sysfetch.c\n");
}

void sealcode(){
    char command[600];
    snprintf(command, sizeof(command), "git clone https://github.com/zilelai/SealCode");
            
    int status = system(command);
    if (status != 0) {
        printf("errcode 21: git clone failed\n");
    }

    system("cd SealCode && gcc main.c -o SealCode");

    printf("SealKernel directories: \n");
    printf("SealCode - /SealCode/main.c\n");
}

void both(){
    char command[600];
    
    snprintf(command, sizeof(command), "git clone https://github.com/zilelai/SealCode");
    if (system(command) != 0) {
        printf("errcode 21: git clone SealCode failed\n");
    }

    snprintf(command, sizeof(command), "git clone https://github.com/zilelai/sysfetch");
    if (system(command) != 0) {
        printf("errcode 21: git clone sysfetch failed\n");
    }

    system("cd SealCode && gcc main.c -o SealCode");
    system("cd sysfetch && gcc sysfetch.c -o sysfetch");

    printf("SealKernel directories: \n");
    printf("sysfetch - /sysfetch/sysfetch.c\n");
    printf("SealCode - /SealCode/SealCode.c\n");
}

void dependencies(){
    printf("Dependencies to choose: sysfetch and SealCode\n");
    printf("1. SealCode\n");
    printf("2. sysfetch\n");
    printf("3. SealCode and sysfetch\n");
    while(1){
        scanf("%s", input);
        if(strcmp(input, "1") == 0){
            sealcode();
            break;
        }
        else if(strcmp(input, "2") == 0){
            sysfetch();
            break;
        }
        else if(strcmp(input, "3") == 0){
            both();
            break;
        }
        else{
            printf("Select again , only 1, 2 and 3 are allowed.\n");
        }
    }
}

int main() {
    printf("Welcome to SealKernel Downloader\n");
    printf("SealKernel Downloader/Installer is where you download your own preference of SealKernel. You can be extremely minimalist or have all of the ttys and bootloaders.\n");
    printf("To start, choose your own SealKernel preference.\n");
    printf("1. only tty1\n");
    printf("2. only tty2\n");
    printf("3. both tty1 and tty2 and also bootloader\n");
    printf("For more information on what tty1 and 2 is, visit zilelai.lab26.my\n\n> ");

    while (1) {
        scanf("%127s", input);
    
        if (strcmp(input, "1") == 0) {
            tty1();
            break;
        } 
        else if (strcmp(input, "2") == 0) {
            tty2();
            break;
        } 
        else if (strcmp(input, "3") == 0) {
            allpackage();
            break;
        } 
        else {
            printf("You can only choose 1, 2 or 3. Select again: ");
        }
    }

    printf("\nFinished downloading shell\n");
    printf("Do you want to install dependencies? (1 for Yes, 2 for No): ");

    while (1) {
        scanf("%127s", input);
    
        if (strcmp(input, "1") == 0) {
            dependencies();
            break;
        } 
        else if (strcmp(input, "2") == 0) {
            printf("Skipping dependencies. Exiting.\n");
        } 
        else {
            printf("You can only choose either 1 or 2. Select again: ");
        }
    }

    printf("Downloading finish.");
    printf("Enjoy SealKernel by using ./ in bash\n");
    
    return 0;
}
