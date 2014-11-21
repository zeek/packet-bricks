#!/usr/bin/env bash
# SCRIPT THAT GENERATES BRICK.C AND BRICK.H FILES. PLEASE DO NOT
# EDIT THIS FILE. YOU MAY RISK BREAKING THE COMPLILATION PROCESS
# IF YOU EDIT/MODIFY THIS FILE
COUNTER=3
cat src/brick.c.in > src/brick.c
cat include/brick.h.in > include/brick.h
while read brickline; do
    if [[ $brickline != \#* ]]; then
	echo -e "\telibs[$COUNTER] = $brickline;" >> src/brick.c
	echo -e "extern brick_funcs $brickline;" >> include/brick.h
	COUNTER=$(($COUNTER + 1))
    fi
done < src/bricks/bricks.in

# Finalize src/brick.c file
echo -e "\t/* delimiter */" >> src/brick.c
echo -e "\telibs[$COUNTER] = (brick_funcs){NULL, NULL, NULL, NULL, NULL};" >> src/brick.c
echo -e "\tTRACE_BRICK_FUNC_END();" >> src/brick.c
echo "}" >> src/brick.c
echo "/*---------------------------------------------------------------------*/" >> src/brick.c

# Finalize include/brick.h file
echo -e "extern brick_funcs elibs[MAX_BRICKS];" >> include/brick.h
echo -e "/*---------------------------------------------------------------------*/" >> include/brick.h
echo -e "#endif /* !__BRICK_H__ */" >> include/brick.h

