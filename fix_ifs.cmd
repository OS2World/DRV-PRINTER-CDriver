/* fix_ifs.cmd - Replace vboxsf.ifs with VBoxFS.ifs in CONFIG.SYS */
parse arg infile outfile
if outfile = '' then outfile = 'CONFIG.SYS.new'

call linein infile, 1, 0  /* reset to start */
do while lines(infile) > 0
  line = linein(infile)
  if pos('vboxsf.ifs', line) > 0 then
    line = translate(line, 'VBoxFS.ifs', 'vboxsf.ifs')
  call lineout outfile, line
end
call lineout infile
call lineout outfile
exit 0