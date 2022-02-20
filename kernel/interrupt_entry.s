# define interrupt that already has error number pushed by the processor
#define DEF_INT_ERR(no) \
.global interrupt_entry_ ## no; \
interrupt_entry_ ## no: \
  pusha; \
  push %esp;  \
  push $no; \
  call interrupt_handler

# push a dummy error code 0 to make the interrupt frame uniform
#define DEF_INT_NOERR(no) \
.global interrupt_entry_ ## no; \
interrupt_entry_ ## no: \
  push $0; \
  pusha; \
  push %esp;  \
  push $no; \
  call interrupt_handler

DEF_INT_NOERR(0)
DEF_INT_NOERR(1)
DEF_INT_NOERR(2)
DEF_INT_NOERR(3)
DEF_INT_NOERR(4)
DEF_INT_NOERR(5)
DEF_INT_NOERR(6)
DEF_INT_NOERR(7)
DEF_INT_ERR(8) # double fault
DEF_INT_NOERR(9)
DEF_INT_ERR(10) # invalid TSS
DEF_INT_ERR(11) # segment not present
DEF_INT_ERR(12) # stack fault
DEF_INT_ERR(13) # general protection
DEF_INT_ERR(14) # page fault
DEF_INT_NOERR(15)
DEF_INT_NOERR(16)
DEF_INT_ERR(17) # alignment check
DEF_INT_NOERR(18)
DEF_INT_NOERR(19)
DEF_INT_NOERR(20)
DEF_INT_ERR(21) # control protection
DEF_INT_NOERR(22)
DEF_INT_NOERR(23)
DEF_INT_NOERR(24)
DEF_INT_NOERR(25)
DEF_INT_NOERR(26)
DEF_INT_NOERR(27)
DEF_INT_NOERR(28)
DEF_INT_NOERR(29)
DEF_INT_NOERR(30)
DEF_INT_NOERR(31)
DEF_INT_NOERR(32)
DEF_INT_NOERR(33)
DEF_INT_NOERR(34)
DEF_INT_NOERR(35)
DEF_INT_NOERR(36)
DEF_INT_NOERR(37)
DEF_INT_NOERR(38)
DEF_INT_NOERR(39)
DEF_INT_NOERR(40)
DEF_INT_NOERR(41)
DEF_INT_NOERR(42)
DEF_INT_NOERR(43)
DEF_INT_NOERR(44)
DEF_INT_NOERR(45)
DEF_INT_NOERR(46)
DEF_INT_NOERR(47)
DEF_INT_NOERR(48)
DEF_INT_NOERR(255)
