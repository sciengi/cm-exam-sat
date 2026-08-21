
import numpy as np
from PySide6.QtWidgets import QWidget, QVBoxLayout
from pyqtgraph.opengl.MeshData import MeshData
import pyqtgraph.opengl as gl


class ViewportWidget(QWidget):
    
    def __init__(self, parent=None):
        super().__init__(parent)
        
        layout = QVBoxLayout(self)
        
        self.view = gl.GLViewWidget()
        self.view.setCameraPosition(distance=4, elevation=30, azimuth=45)
        layout.addWidget(self.view)
        
        grid = gl.GLGridItem()
        grid.setSize(x=2, y=2, z=2)
        grid.setSpacing(x=0.2, y=0.2, z=0.2)
        self.view.addItem(grid)

        # TODO: learn about pyqtgraph
        # TODO: move to "AtomRepresentation" class
        
        sphere_mesh_data = MeshData.sphere(rows=12, cols=12, radius=0.01)
        sphere_mesh_data.faceNormals()
        self.base_verts = sphere_mesh_data.vertexes()
        self.base_faces = sphere_mesh_data.faces()

        self.mesh_item = gl.GLMeshItem(
            shader='shaded',
            smooth=True,
            glOptions='opaque'
        )
        self.mesh_item.setVisible(False)
        self.view.addItem(self.mesh_item)
        
    def update(self, event: dict):
        centers = event['data'].reshape((-1, 3))  # DEV: suppose that dim = 3
        centers = centers[:len(centers) // 2]     # TODO: add checkbox to visualize speeds
        # DEV: system vector contains postion and velocity in sequence,
        #      i.e. [x1, y1, z1, ..., vx1, vy1, vz1, ...]

        if len(centers) == 0:  # TODO: event is correct by default, is this need?
            self.mesh_item.setVisible(False)
            self.mesh_item.update()
            return

        self.mesh_item.setVisible(True)

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

        md = MeshData(
            vertexes=final_verts, 
            faces=final_faces, 
            faceColors=final_colors
        )
        
        md.faceNormals()
        md.vertexNormals()
        
        self.mesh_item.setMeshData(meshdata=md)
        self.mesh_item.opts['lightPos'] = (5, 5, 10)
        self.mesh_item.update()
        