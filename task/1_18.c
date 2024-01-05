#include <stdio.h>
int main(){
volatile long sum = 0;
long i;for(i = 0; i < 3360000000; i++){
sum++;}system("touch 1_18.txt");return 0;}
