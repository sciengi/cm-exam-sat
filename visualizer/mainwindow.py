
import numpy as np

from PySide6.QtWidgets import QMainWindow, QWidget, QVBoxLayout, QPushButton
from pyqtgraph.opengl.MeshData import MeshData
import pyqtgraph.opengl as gl


class MainWindow(QMainWindow):
    
    # TODO: add logging 
    
    def __init__(self):
        super().__init__()
        
        self.setWindowTitle("Visualizer Prototype")
        self.resize(800, 600)

        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)

        # TODO: move to Viewport widget
        self.view = gl.GLViewWidget()
        self.view.setCameraPosition(distance=3, elevation=30, azimuth=45)
        layout.addWidget(self.view)

        grid = gl.GLGridItem()
        grid.setSize(x=2, y=2, z=2)
        grid.setSpacing(x=0.2, y=0.2, z=0.2)
        self.view.addItem(grid)

        # TODO: move to "AtomRepresentation" class
        sphere_mesh_data = MeshData.sphere(rows=12, cols=12, radius=0.01)
        self.base_verts = sphere_mesh_data.vertexes()
        self.base_faces = sphere_mesh_data.faces()

        self.mesh_item = gl.GLMeshItem(
            shader='shaded',
            smooth=True,
            glOptions='opaque'
        )
        self.view.addItem(self.mesh_item)

        # TODO: move to ComandPanel widget
        self.btn_generate = QPushButton("reset")
        self.btn_generate.setStyleSheet("font-size: 14px; padding: 10px;")
        self.btn_generate.clicked.connect(self.generate_and_update)
        layout.addWidget(self.btn_generate)

        self.generate_and_update()  

    def generate_and_update(self):
        
        # TODO: split generation (that will be reading from solver) and update
        # TODO(future): read about gl routines: how to render effective 
        
        centers = np.random.uniform(-1., 1., (100, 3))

        all_verts = []
        all_faces = []
        all_colors = []

        base_color = [0.1, 0.9, 0.5, 1.0]

        for i, center in enumerate(centers):
            shifted_verts = self.base_verts + center
            all_verts.append(shifted_verts)

            shifted_faces = self.base_faces + (i * len(self.base_verts))
            all_faces.append(shifted_faces)

            num_faces = len(self.base_faces)
            all_colors.append([base_color] * num_faces)

        final_verts = np.vstack(all_verts)
        final_faces = np.vstack(all_faces)
        final_colors = np.vstack(all_colors)

        md = MeshData(vertexes=final_verts, faces=final_faces, faceColors=final_colors)  # DEV: MeshData for normals calculation
        
        self.mesh_item.setMeshData(meshdata=md)
        self.mesh_item.opts['lightPos'] = (5, 5, 10)
        self.mesh_item.update()
        