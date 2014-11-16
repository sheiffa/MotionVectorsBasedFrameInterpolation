LoadPlugin("c:\Workspace\inz\dev\Debug\mvtools2debug.dll")
source=DirectShowSource("C:\Workspace\inz\samples\water.mov",fps=50, audio=false)
super = MSuper(source)

backward_vec = MAnalyse(super, chroma=false, isb=true, blksize=16, blksizev=16, searchparam=2, plevel=0, search=3, badrange=(-24))
forward_vec = MAnalyse(super, chroma=false, isb=false, blksize=16, blksizev=16, searchparam=2, plevel=0, search=3, badrange=(-24))

BlockFpsWithCorrectionVectors(source, super, backward_vec, forward_vec, source, 50, 2,0)