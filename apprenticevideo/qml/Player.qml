import QtQuick 2.4
import QtQml 2.2
import QtQml.Models 2.2
import com.aragog.apprenticevideo 1.0
import '.'

Item
{
  id: player
  objectName: "player"

  signal exitFullScreen()
  signal toggleFullScreen()
  signal skipForward()
  signal skipBack()
  signal stepOneFrameForward()

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

  MouseArea
  {
    anchors.fill: parent
    anchors.margins: 0
    // propagateComposedEvents: true
    // preventStealing: true

    onDoubleClicked: {
      console.log("Player: DOUBLE CLICKED!")
      toggleFullScreen();
      mouse.accepted = true;
    }
  }

  Playlist
  {
    id: playlist
    objectName: "playlist"
    anchors.fill: parent
    anchors.margins: 0
  }

}
