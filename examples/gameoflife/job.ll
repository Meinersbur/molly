# @ job_name = gameoflife
# @ comment = "Molly example gameoflife"
# @ error = $(job_name).$(jobid).out
# @ output = $(job_name).$(jobid).out
# @ environment = COPY_ALL;
# @ wall_clock_limit = 00:30:00
# @ notification = error
# @ notify_user = juqueen@meinersbur.de
# @ job_type = bluegene
# @ bg_size = 64
# @ queue

echo @bg_size = 64
PROG="runjob --np 65 --env-all --ranks-per-node 1 : gameoflife8_64"

echo "${PROG}"
${PROG}

