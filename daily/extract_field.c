
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* origin
char *get_one_section_from_hash_value(char **value)
{
    int i = 0;
    int is_find = 0;
    int len = strlen(*value);
    char *dsl_name = *value;
    for (i = 0; i < len; i++)
    {
        printf("i1=%d\n", i);
        if ((*value)[i] == ';')
        {
            is_find = 1;
            printf("i2=%d\n", i);
            (*value)[i] = '\0';
            printf("i=%d\n", i);
            *value += strlen(dsl_name) + 1;
            break;
        }
    }
    if (is_find != 1)
    {
        if (strlen(*value) <= 0)
        {
            dsl_name = NULL;
        }
        else
        {
            *value += strlen(dsl_name);
        }
    }

    return dsl_name;
}
*/

/* refactor */
char *get_one_section_from_hash_value(char **value)
{
    int len = strlen(*value), offset = 0;
    char *dsl_name = *value;
    char *pos = strchr(*value, ';');
    if (pos != NULL)
    {
        offset = pos - (*value);
        (*value)[offset] = '\0';     
        *value += offset + 1; 
    }
    else
    {
        *value += len;
    }
    return dsl_name;
}


int main(int argc, char *argv[])
{
    char *str = malloc(20);
    strcpy(str, "ABCDEFG;");
    char *res;
    res = get_one_section_from_hash_value(&str);
    printf("result=%s\n", res); 
    //free(str);
    return 0;
}
