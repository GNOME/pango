#!/bin/sh

echo -n > table.tmp

for i in maps/* ; do
  name=`basename $i`
  case $name in
    README | CVS)
	;;
    *)	
	cat $i | grep -v '^#' | sed -e 's/[:space:]*#.*$//' | awk "{ printf \"%s %s:%s\\n\", \$NF, \"$name\", \$1 }" >> table.tmp
	;;
  esac
done
sort table.tmp > table
rm table.tmp
