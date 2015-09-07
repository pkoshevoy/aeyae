import QtQuick 2.4
import QtQml 2.2
import QtQml.Models 2.2
import com.aragog.apprenticevideo 1.0
import '.'
import 'Utils.js' as Utils

Item
{
  id: timeline
  objectName: "timeline"

  Rectangle
  {
    anchors.fill: parent
    anchors.margins: 0
    anchors.topMargin: -(parent.height * 2)

    gradient: Gradient {
      GradientStop { position: 0.000000; color: '#01000000'; }
      GradientStop { position: 0.010417; color: '#01000000'; }
      GradientStop { position: 0.020833; color: '#01000000'; }
      GradientStop { position: 0.031250; color: '#01000000'; }
      GradientStop { position: 0.041667; color: '#01000000'; }
      GradientStop { position: 0.052083; color: '#02000000'; }
      GradientStop { position: 0.062500; color: '#02000000'; }
      GradientStop { position: 0.072917; color: '#02000000'; }
      GradientStop { position: 0.083333; color: '#03000000'; }
      GradientStop { position: 0.093750; color: '#03000000'; }
      GradientStop { position: 0.104167; color: '#03000000'; }
      GradientStop { position: 0.114583; color: '#03000000'; }
      GradientStop { position: 0.125000; color: '#03000000'; }
      GradientStop { position: 0.135417; color: '#04000000'; }
      GradientStop { position: 0.145833; color: '#04000000'; }
      GradientStop { position: 0.156250; color: '#05000000'; }
      GradientStop { position: 0.166667; color: '#06000000'; }
      GradientStop { position: 0.177083; color: '#06000000'; }
      GradientStop { position: 0.187500; color: '#07000000'; }
      GradientStop { position: 0.197917; color: '#07000000'; }
      GradientStop { position: 0.208333; color: '#08000000'; }
      GradientStop { position: 0.218750; color: '#09000000'; }
      GradientStop { position: 0.229167; color: '#0a000000'; }
      GradientStop { position: 0.239583; color: '#0b000000'; }
      GradientStop { position: 0.250000; color: '#0b000000'; }
      GradientStop { position: 0.260417; color: '#0c000000'; }
      GradientStop { position: 0.270833; color: '#0d000000'; }
      GradientStop { position: 0.281250; color: '#0f000000'; }
      GradientStop { position: 0.291667; color: '#0f000000'; }
      GradientStop { position: 0.302083; color: '#10000000'; }
      GradientStop { position: 0.312500; color: '#12000000'; }
      GradientStop { position: 0.322917; color: '#13000000'; }
      GradientStop { position: 0.333333; color: '#14000000'; }
      GradientStop { position: 0.343750; color: '#16000000'; }
      GradientStop { position: 0.354167; color: '#17000000'; }
      GradientStop { position: 0.364583; color: '#18000000'; }
      GradientStop { position: 0.375000; color: '#1a000000'; }
      GradientStop { position: 0.385417; color: '#1b000000'; }
      GradientStop { position: 0.395833; color: '#1d000000'; }
      GradientStop { position: 0.406250; color: '#1f000000'; }
      GradientStop { position: 0.416667; color: '#21000000'; }
      GradientStop { position: 0.427083; color: '#22000000'; }
      GradientStop { position: 0.437500; color: '#24000000'; }
      GradientStop { position: 0.447917; color: '#26000000'; }
      GradientStop { position: 0.458333; color: '#28000000'; }
      GradientStop { position: 0.468750; color: '#2a000000'; }
      GradientStop { position: 0.479167; color: '#2c000000'; }
      GradientStop { position: 0.489583; color: '#2e000000'; }
      GradientStop { position: 0.500000; color: '#31000000'; }
      GradientStop { position: 0.510417; color: '#33000000'; }
      GradientStop { position: 0.520833; color: '#35000000'; }
      GradientStop { position: 0.531250; color: '#38000000'; }
      GradientStop { position: 0.541667; color: '#3a000000'; }
      GradientStop { position: 0.552083; color: '#3d000000'; }
      GradientStop { position: 0.562500; color: '#40000000'; }
      GradientStop { position: 0.572917; color: '#42000000'; }
      GradientStop { position: 0.583333; color: '#44000000'; }
      GradientStop { position: 0.593750; color: '#47000000'; }
      GradientStop { position: 0.604167; color: '#49000000'; }
      GradientStop { position: 0.614583; color: '#4c000000'; }
      GradientStop { position: 0.625000; color: '#4f000000'; }
      GradientStop { position: 0.635417; color: '#52000000'; }
      GradientStop { position: 0.645833; color: '#54000000'; }
      GradientStop { position: 0.656250; color: '#58000000'; }
      GradientStop { position: 0.666667; color: '#5a000000'; }
      GradientStop { position: 0.677083; color: '#5d000000'; }
      GradientStop { position: 0.687500; color: '#60000000'; }
      GradientStop { position: 0.697917; color: '#63000000'; }
      GradientStop { position: 0.708333; color: '#64000000'; }
      GradientStop { position: 0.718750; color: '#67000000'; }
      GradientStop { position: 0.729167; color: '#6a000000'; }
      GradientStop { position: 0.739583; color: '#6d000000'; }
      GradientStop { position: 0.750000; color: '#6f000000'; }
      GradientStop { position: 0.760417; color: '#72000000'; }
      GradientStop { position: 0.770833; color: '#75000000'; }
      GradientStop { position: 0.781250; color: '#78000000'; }
      GradientStop { position: 0.791667; color: '#7a000000'; }
      GradientStop { position: 0.802083; color: '#7e000000'; }
      GradientStop { position: 0.812500; color: '#80000000'; }
      GradientStop { position: 0.822917; color: '#83000000'; }
      GradientStop { position: 0.833333; color: '#85000000'; }
      GradientStop { position: 0.843750; color: '#87000000'; }
      GradientStop { position: 0.854167; color: '#89000000'; }
      GradientStop { position: 0.864583; color: '#8d000000'; }
      GradientStop { position: 0.875000; color: '#8f000000'; }
      GradientStop { position: 0.885417; color: '#91000000'; }
      GradientStop { position: 0.895833; color: '#94000000'; }
      GradientStop { position: 0.906250; color: '#96000000'; }
      GradientStop { position: 0.916667; color: '#98000000'; }
      GradientStop { position: 0.927083; color: '#9b000000'; }
      GradientStop { position: 0.937500; color: '#9e000000'; }
      GradientStop { position: 0.947917; color: '#a1000000'; }
      GradientStop { position: 0.958333; color: '#a4000000'; }
      GradientStop { position: 0.968750; color: '#a7000000'; }
      GradientStop { position: 0.979167; color: '#aa000000'; }
      GradientStop { position: 0.989583; color: '#ad000000'; }
      GradientStop { position: 1.000000; color: '#b0000000'; }
    }
  }

  Item
  {
    id: container
    objectName: "container"

    anchors.fill: parent
    anchors.margins: 0
    anchors.leftMargin: height / 3
    anchors.rightMargin: height / 3

    function timeline_height_thin()
    {
      var h = this.height / 18.0
      return h;
    }

    function timeline_height_thick()
    {
      var h = this.height / 9.0
      return h;
    }

    function timeline_height()
    {
      var h =
          mouseArea.containsMouse ?
          timeline_height_thick() :
          timeline_height_thin();
      return h;
    }

    MouseArea
    {
      id: mouseArea
      hoverEnabled: true

      anchors.left: parent.left
      anchors.right: parent.right
      anchors.top: parent.top
      anchors.bottom: parent.top
      anchors.leftMargin: -(parent.height / 3.0)
      anchors.rightMargin: -(parent.height / 3.0)

      // NOTE: it appears mouse hover is skewed
      // possibly due to arrow cursor hot spot not being at (0, 0),
      // therefore the bottom margin is artificially padded by 4 pixels:
      anchors.topMargin: -(parent.height / 9.0)
      anchors.bottomMargin: -(parent.height / 9.0 + 4)

      /*
      // FIXME: for hover area testing
      Rectangle
      {
        anchors.fill: parent
        anchors.margins: 0
        color: "#3f00ff00"
      }
      */

      onDoubleClicked: {
        // Utils.dump_properties(mouseArea);
        var padding = (parent.height / 3.0)
        var t = (mouse.x - padding) / (this.width - padding * 2.0);
        yae_timeline_controls.markerPlayhead = t;
        mouse.accepted = true;
      }
    }

    Rectangle
    {
      id: timelineIn
      objectName: "timelineIn"
      color: "#33ffffff"
      y: -this.height / 2.0
      anchors.left: parent.left
      height: container.timeline_height();
      width: parent.width * yae_timeline_controls.markerTimeIn;
    }

    Rectangle
    {
      id: timelinePlayhead
      objectName: "timelinePlayhead"
      color: "#f12b24"
      y: -this.height / 2.0
      anchors.left: timelineIn.right
      height: timelineIn.height
      width: parent.width * (yae_timeline_controls.markerPlayhead -
                             yae_timeline_controls.markerTimeIn);
    }

    Rectangle
    {
      id: timelineOut
      objectName: "timelineOut"
      color: "#84ffffff"
      y: -this.height / 2.0
      anchors.left: timelinePlayhead.right
      height: timelineIn.height
      width: parent.width * (yae_timeline_controls.markerTimeOut -
                             yae_timeline_controls.markerPlayhead);
    }

    Rectangle
    {
      id: timelineEnd
      objectName: "timelineEnd"
      color: "#33ffffff"
      y: -this.height / 2.0
      anchors.left: timelineOut.right
      anchors.right: parent.right
      height: timelineIn.height
    }

    TimelineMarkerDragItem
    {
      id: "dragItem"
      objectName: "dragItem"
    }

    TimelineMarker
    {
      id: inPoint
      objectName: "inPoint"
      color: timelinePlayhead.color
      position: timelinePlayhead
      container: container
      dragItem: dragItem
      height: container.timeline_height_thick() * 1.67
      visible: mouseArea.containsMouse

      moveMarker: function(t) {
        yae_timeline_controls.markerTimeIn = t;
      }
    }

    TimelineMarker
    {
      id: outPoint
      objectName: "outPoint"
      color: "#e6e6e6"
      position: timelineEnd
      container: container
      dragItem: dragItem
      height: inPoint.height
      visible: mouseArea.containsMouse

      moveMarker: function(t) {
        yae_timeline_controls.markerTimeOut = t;
      }
    }

    TimelineMarker
    {
      id: playhead
      objectName: "playhead"
      color: timelinePlayhead.color
      position: timelineOut
      container: container
      dragItem: dragItem
      height: container.timeline_height_thick() * 2.0
      visible: mouseArea.containsMouse

      moveMarker: function(t) {
        yae_timeline_controls.markerPlayhead = t;
      }
    }

    TextInput
    {
      id: playheadAux
      objectName: "playheadAux"

      property var timestamp: yae_timeline_controls.auxPlayhead

      anchors.left: parent.left
      anchors.verticalCenter: parent.verticalCenter
      anchors.leftMargin: 3
      anchors.rightMargin: 3

      text: (activeFocus ? timestamp : yae_timeline_controls.auxPlayhead)

      font.family: ("Droid Sans Mono, DejaVu Sans Mono, " +
                    "Bitstream Vera Sans Mono, Consolas, " +
                    "Lucida Console, Lucida Sans Typewriter, " +
                    "Menlo, Monaco, Courier New, Courier, " +
                    "monospace")
      font.pixelSize: parent.height / 3

      color: "#7fffffff"
      selectByMouse: true

      onFocusChanged: if (activeFocus) {
        timestamp = yae_timeline_controls.auxPlayhead;
      }

      KeyNavigation.tab: timeline.parent
      KeyNavigation.backtab: timeline.parent

      onAccepted: {
        yae_timeline_controls.auxPlayhead = text;
        timeline.parent.focus = true;
      }

      Rectangle
      {
        z: -1
        anchors.fill: parent
        anchors.margins: 0
        anchors.leftMargin: -3
        anchors.rightMargin: -3

        color: "#3f7f7f7f"
        radius: 3
      }
    }

    Text
    {
      id: durationAux
      objectName: "durationAux"

      anchors.right: parent.right
      anchors.verticalCenter: parent.verticalCenter
      anchors.leftMargin: 3
      anchors.rightMargin: 3

      text: yae_timeline_controls.auxDuration
      color: "#7fffffff"
      font: playheadAux.font

      Rectangle
      {
        z: -1
        anchors.fill: parent
        anchors.margins: 0
        anchors.leftMargin: -3
        anchors.rightMargin: -3

        color: "#3f7f7f7f"
        radius: 3
      }
    }
  }
}
