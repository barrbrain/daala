{
echo '<pre>'
cat last_run
./tools/bd_rate.sh master.out `tr ' ' _ < last_run`.out
echo '</pre>'
echo '<div><img src="psnr.png"/></div>'
echo '<div><img src="psnrhvs.png"/></div>'
echo '<div><img src="ssim.png"/></div>'
echo '<div><img src="fastssim.png"/></div>'
} > ~/htdocs/index.html.new
if [ "`sha1sum - < ~/htdocs/index.html.new`" = "`sha1sum - < ~/htdocs/index.html`" ]
then
  exit 0
fi

mv ~/htdocs/index.html.new ~/htdocs/index.html
./tools/rd_plot.sh master.out `tr ' ' _ < last_run`.out
mv *.png ~/htdocs

