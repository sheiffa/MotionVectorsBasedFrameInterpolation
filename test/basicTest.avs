LoadPlugin("..\Debug\mvtools2debug.dll")
DirectShowSource("C:\Workspace\inz\samples\closeup2.mov",fps=50, audio=false)
super = MSuper(pel=2, hpad=0, vpad=0)

backward_1 = MAnalyse(super, chroma=false, isb=true, blksize=16, blksizev=16, searchparam=2, plevel=0, search=3, badrange=(-24))
forward_1 = MAnalyse(super, chroma=false, isb=false, blksize=16, blksizev=16, searchparam=2, plevel=0, search=3, badrange=(-24))

errors = AnalyseErrors(super, 2, 50)

BlockFpsWithCorrectionVectors(super, backward_1, forward_1, errors, 50, 4,0)