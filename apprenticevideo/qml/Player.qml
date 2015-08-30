import QtQuick 2.4
import QtQml 2.2
import QtQml.Models 2.2
import com.aragog.apprenticevideo 1.0
import '.'

Item
{
  id: player
  objectName: "player"

  CanvasQuickFbo
  {
    id: renderer
    objectName: "renderer"

    anchors.fill: parent
    anchors.margins: 0

    // flip it right-side-up:
    transform:
    [
      Scale { yScale: -1; },
      Translate { y: renderer.height; }
    ]
  }

  Playlist {
    id: playlist
    objectName: "playlist"
    anchors.fill: parent
  }
}
