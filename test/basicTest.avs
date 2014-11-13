LoadPlugin("c:\Workspace\inz\dev\Debug\mvtools2debug.dll")
source=DirectShowSource("C:\Workspace\inz\samples\water.mov",fps=50, audio=false)
super = MSuper(source)

backward_vec = MAnalyse(super, overlap=4, isb = true, search=3)
# Use block overlap, halfpixel accuracy and Exhaustive search
forward_vec = MAnalyse(super, overlap=4, isb = false, search=3)

errors=AnalyseErrors(source)

BlockFpsWithCorrectionVectors(source, super, backward_vec, forward_vec, errors, 2)