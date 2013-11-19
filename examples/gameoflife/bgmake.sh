#! /bin/bash

MPIFLAGS="-I/bgsys/drivers/V1R2M1/ppc64/comm/lib/gnu -I/bgsys/drivers/V1R2M1/ppc64 -I/bgsys/drivers/V1R2M1/ppc64/comm/sys/include -I/bgsys/drivers/V1R2M1/ppc64/spi/include -I/bgsys/drivers/V1R2M1/ppc64/spi/include/kernel/cnk -L/bgsys/drivers/V1R2M1/ppc64/comm/lib -L/bgsys/drivers/V1R2M1/ppc64/comm/lib -L/bgsys/drivers/V1R2M1/ppc64/comm/lib64 -L/bgsys/drivers/V1R2M1/ppc64/comm/lib -L/bgsys/drivers/V1R2M1/ppc64/spi/lib -L/bgsys/drivers/V1R2M1/ppc64/comm/sys/lib -L/bgsys/drivers/V1R2M1/ppc64/spi/lib -L/bgsys/drivers/V1R2M1/ppc64/comm/sys/lib -L/bgsys/drivers/V1R2M1/ppc64/comm/lib64 -L/bgsys/drivers/V1R2M1/ppc64/comm/lib -L/bgsys/drivers/V1R2M1/ppc64/spi/lib -I/bgsys/drivers/V1R2M1/ppc64/comm/include -L/bgsys/drivers/V1R2M1/ppc64/comm/lib -lmpichcxx-gcc -Wl,-rpath -Wl,/bgsys/drivers/V1R2M1/ppc64/comm/sys/lib -Wl,-rpath -Wl,/bgsys/drivers/V1R2M1/ppc64/comm/lib -lmpich-gcc -lopa-gcc -lmpl-gcc -lpami-gcc -lSPI -lSPI_cnk -lrt -lpthread -lstdc++ -lpthread"
OPTFLAGS="-O3 -DNDEBUG -g"
bgmollycc ${OPTFLAGS} gameoflife.cpp -o gameoflife8_64 -I/homea/hch02/hch02d/src/molly/molly/include/mollyrt/ -fno-exceptions -mllvm -debug -mllvm -debug-only=polly -mllvm -molly -mllvm -shape=8x8 -mllvm -polly-report -L/homea/hch02/hch02d/src/molly/libcxx/lib /homea/hch02/hch02d/src/molly/molly/lib/mollyrt/molly.a ${MPIFLAGS}
