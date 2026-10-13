import pcbnew
p="C:/LocalProjects/CameraSlider/PCB/CameraSlider/CameraSlider.kicad_pcb"
b=pcbnew.LoadBoard(p)
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
pcbnew.SaveBoard(p,b)
print("filled")
