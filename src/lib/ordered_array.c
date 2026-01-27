#include "ordered_array.h"

bool standard_criteria(type_t a, type_t b)
{
    return a < b;
}

ordered_array create_static_array(void* addr, uint32_t max_size, criteria function)
{
    ordered_array ret;

    ret.array = addr;
    if(function == NULL)
        ret.criteria_function = standard_criteria;
    else
        ret.criteria_function = function;

    ret.max_size = max_size;
    ret.size = 0;

    return ret;
}

ordered_array create_dynamic_array(uint32_t max_size, criteria function)
{
    ordered_array ret;

//    ret.array = malloc(max_size);

    if(function == NULL)
        ret.criteria_function = standard_criteria;
    else
        ret.criteria_function = function;

    ret.max_size = max_size;
    ret.size = 0;

    return ret;
}

uint32_t insert_ordered_array(type_t item, ordered_array *array)
{
    uint32_t iterator = 0;
    uint32_t ret = 0;

    if(array->size >= array->max_size)
        return array->max_size; // error
    
    while (iterator < array->size && array->criteria_function(array->array[iterator], item))
        iterator++;
    
    ret = iterator; // we will return the index where to put the item !

    if(iterator == array->size)
        array->array[array->size++] = item;
    else
    {
        type_t temp = array->array[iterator];
        array->array[iterator] = item;

        while (iterator < array->size)
        {
            iterator++;
            type_t temp2 = array->array[iterator];
            array->array[iterator] = temp;
            temp = temp2;
        }

        iterator++;
        array->array[iterator] = temp;
        array->size++;
    }
    
    return ret;
}

type_t lookup_ordered_array(uint32_t index, ordered_array *array)
{
    if(index >= array->size)
        return NULL;
    
    return array->array[index];
}

void remove_ordered_array(uint32_t index, ordered_array *array)
{
    if(index >= array->size)
        return;

    while(index < array->size)
    {
        array->array[index] = array->array[index+1];
        index++;
    }

    array->size--;
}

uint32_t getIndex_ordered_array(type_t item, ordered_array *array)
{
    for(int i = 0; i < array->size; i++)
    {
        if(array->array[i] == item)
            return i;
    }

    return array->max_size; // error !
}