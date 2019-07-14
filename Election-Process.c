#include <stdio.h>
#include "mpi.h"
#include <stdlib.h>
void shuffle(int *arr, int n)
{
    int temp, idx1, idx2, i;
    for (i = 0; i < n; i++)
    {
        idx1 = rand() % n;
        idx2 = rand() % n; // swap two random elements
        int temp = arr[idx1];
        arr[idx1] = arr[idx2];
        arr[idx2] = temp;
    }
}
int getFirstTwo(int c, int v)
{
    int length = 0;
    char *s;
    s = malloc(sizeof(char) * c);
    sprintf(s, "%d", c);
    length += strlen(s);
    s = malloc(sizeof(char) * v);
    sprintf(s, "%d", v);
    length += strlen(s);
    length += 2;
    return length;
}
int getLineLength(int x)
{
    int i;
    int length = 0;
    char *s;

    for (i = 1; i <= x; i++)
    {
        s = malloc(sizeof(char) * i + 1);
        sprintf(s, "%d", i);
        length += strlen(s);
        length++; //the number of spaces and the new line
    }
    return length + 1;
}
int main(int argc, char *argv[])
{
    int my_rank, p, temp, length, c, v, portion, rem, i, j, f2, tag = 0;
    FILE *fptr;
    int *localArr, *localRemainder; //will hold the contents of each file portion
    int *globalWin, *localWin;
    int scnd = 1;
    int winnerIdx = -1, secondWinnerIdx = -1, globalFirst, globalSecond;
    ;
    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    if (my_rank == 0)
    {
        printf("Enter the number of Candidates: ");
        scanf("%d", &c);
        printf("Enter the number of Voters: ");
        scanf("%d", &v); //number of rows
        portion = v / p;
        rem = v % p;
        length = getLineLength(c);
        f2 = getFirstTwo(c, v);
        printf("The line length is %d \n", length);
        printf("The FirstTwolines length is %d \n", f2);

        int *tempArr = malloc(sizeof(int) * c);
        globalWin = malloc(sizeof(int) * c);
        for (i = 0; i < c; i++)
            tempArr[i] = i + 1;

        fptr = fopen("joe.txt", "w");
        fprintf(fptr, "%d\n", c); //no of candidates
        fprintf(fptr, "%d\n", v); //no of voters

        //Writes numbers to File
        for (i = 0; i < v; i++)
        {
            shuffle(tempArr, c);
            for (j = 0; j < c; j++)
                fprintf(fptr, "%d ", tempArr[j]);
            fprintf(fptr, "\n");
        }
        fclose(fptr);
    }

    MPI_Bcast(&portion, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&v, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&c, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&length, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&f2, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //Here I want to read specific amount from the file, then prints it
    fptr = fopen("joe.txt", "r");
    fseek(fptr, my_rank * length * portion + f2, SEEK_SET);
    localArr = malloc(sizeof(int) * portion * c);
    for (i = 0; i < portion * c; i++)
    {
        fscanf(fptr, "%d", &localArr[i]);
    }
    fclose(fptr);

    //I want to count the first index of each voter
    localWin = malloc(sizeof(int) * c);
    memset(localWin, 0, sizeof(int) * c);
    printf("Process %d\n", my_rank);

    for (i = 0; i < portion; i++)
    {
        for (j = 0; j < c; j++)
        {
            if (j == 0)
                localWin[localArr[i * c + j] - 1]++;
            printf("%d ", localArr[i * c + j]);
        }
        printf("\n");
    }

    MPI_Reduce(localWin, globalWin, c, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (my_rank == 0)
    {

        fptr = fopen("joe.txt", "r");
        //The remainig part, root 0 will put again into localArr but after being freed
        fseek(fptr, p * length * portion + f2, SEEK_SET); // seek to no of process * length of each row
        localRemainder = malloc(sizeof(int) * rem * c);   //allocate space for rem rows and c cols
        for (i = 0; i < c * rem; i++)
            fscanf(fptr, "%d", &localRemainder[i]);
        fclose(fptr);
        printf("Remaining part from Process %d\n", my_rank);

        for (i = 0; i < rem; i++)
        {

            for (j = 0; j < c; j++)
            {
                if (j == 0)
                    globalWin[localRemainder[i * c + j] - 1]++;
                printf("%d ", localRemainder[i * c + j]);
            }
            printf("\n");
        }

        float maxi = 1e-9;

        printf("I'm root and I'm printing the finals of ROUND 1 \n");
        for (i = 0; i < c; i++)
        {
            float res = (float)((globalWin[i] * 100) / v);
            printf("Candidate [%d] got %d/%d which is %f%%\n", i + 1, globalWin[i], v, res);
            if (res >= maxi)
            {
                maxi = res;
                secondWinnerIdx = winnerIdx;
                winnerIdx = i;
            }
        }
        if (maxi >= 50)
        {
            printf("The winner is candidate [%d]\n", winnerIdx + 1);
            scnd = -1;
        }
        else
        {
            printf("No winner in the first round\n");
        }
    }
    //Handling the Second round if any
    MPI_Bcast(&scnd, 1, MPI_INT, 0, MPI_COMM_WORLD);
    int first = 0, second = 0;
    if (scnd != -1)
    {
        //if there's a second round then broadcast the previous winners
        MPI_Bcast(&secondWinnerIdx, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&winnerIdx, 1, MPI_INT, 0, MPI_COMM_WORLD);

        //will search for each local Arr I have for first occurence of any of them
        for (i = 0; i < portion; i++)
        {
            for (j = 0; j < c; j++)
            {
                if (localArr[i * c + j] == winnerIdx + 1)
                {
                    first++;
                    break;
                }
                else if (localArr[i * c + j] == secondWinnerIdx + 1)
                {
                    second++;
                    break;
                }
            }
        }
        MPI_Reduce(&first, &globalFirst, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&second, &globalSecond, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

        if (my_rank == 0)
        {
            for (i = 0; i < rem; i++)
            {
                for (j = 0; j < c; j++)
                {
                    if (localRemainder[i * c + j] == winnerIdx + 1)
                    {
                        globalFirst++;
                        break;
                    }
                    else if (localRemainder[i * c + j] == secondWinnerIdx + 1)
                    {
                        globalSecond++;
                        break;
                    }
                }
            }
            printf("I'm root and I'm printing the finals of ROUND 2 \n");

            float res = (float)((globalFirst * 100) / v);

            printf("Candidate [%d] got %d/%d which is %f%%\n", winnerIdx + 1, globalFirst, v, res);
            res = (float)((globalSecond * 100) / v);
            printf("Candidate [%d] got %d/%d which is %f%%\n", secondWinnerIdx + 1, globalSecond, v, res);
            printf("So The winner is candidate %d\n", globalFirst >= globalSecond ? winnerIdx + 1 : secondWinnerIdx + 1);
        }
    }
    else
    {
        MPI_Finalize();
    }
    MPI_Finalize();
   
}
