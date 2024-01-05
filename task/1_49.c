#include <stdio.h>
int main(){
volatile long sum = 0;
long i;for(i = 0; i < 3170000000; i++){
sum++;}system("touch 1_49.txt");return 0;}
