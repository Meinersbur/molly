# @ job_name = {jobname}
# @ comment = "Molly example gameoflife"
# @ error = {outputfilepath}
# @ output = {outputfilepath}
# @ environment = COPY_ALL;
# @ wall_clock_limit = 00:30:00
# @ notification = error
# @ notify_user = juqueen@meinersbur.de
# @ job_type = bluegene
# @ bg_size = 32
# @ queue

runjob -n 4 --ranks-per-node 1 --env-all $MAPPING : {exe1}
