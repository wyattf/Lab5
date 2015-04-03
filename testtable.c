#include <stdio.h>
#include "main.h"
#include "link.h"
#include "switch.h"

main()
{
    Table ftable;
    
    tableInit(&ftable);
    tableDisplay(&ftable);
    printf("Size of table: %d\n\n", ftable.size);

    tableUpdate(&ftable, 1, 3, 2);
    tableUpdate(&ftable, 1, 2, 1);
    tableUpdate(&ftable, 1, 4, 0);

    tableDisplay(&ftable);
    printf("Size of table: %d\n\n", ftable.size);

    tableUpdate(&ftable, 1, 3, 3);

    printf("Updated table:\n");
    tableDisplay(&ftable);
    printf("Size of table: %d\n\n", ftable.size);

    printf("Out link for dstaddr=2: %d\n", tableGetOutLink(&ftable, 2));
    printf("Out link for dstaddr=3: %d\n", tableGetOutLink(&ftable, 3));
    printf("Out link for dstaddr=4: %d\n", tableGetOutLink(&ftable, 4));

}
