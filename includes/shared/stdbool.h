#ifndef __SDC51_STDBOOL_H
#define __SDC51_STDBOOL_H 1

/* Define true and false of type _Bool in a way compatible with the preprocessor (see N 2229 for details). */
#define true ((_Bool)+1)
#define false ((_Bool)+0)

#define bool _Bool
#define __bool_true_false_are_defined 1

#endif
