#include <stdio.h>

#include "xively.h"

int main()
{
	struct data_points* data;
	
	data=append_data_point(NULL,"id_1","123124");
	data=append_data_point(data,"id_2","1.344");
	data=append_data_point(data,"id_3333333","adfasfafasffasdfa");
	
	update_feed("93279842379","ajdvclsaudiavisdhvlbisaudliudafb34223",data);
	
	return 0;
}