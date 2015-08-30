/*
  A set of stateless helper functions that take input and compute output,
  but never manipulate QML component instances directly.

  As it would be wasteful for each QML component instance to have
  a unique copy of these libraries, the JavaScript programmer can
  indicate a particular file is a stateless library through
  the use of a pragma.
*/
  .pragma library

function dump_properties(item, indentation)
{
  if (!indentation)
  {
    indentation = "";
  }

  console.log("\n\n" + indentation + item);
  for (var p in item)
  {
    if (typeof(item[p]) == "function")
    {
      continue;
    }

    console.log(indentation + p + ": " + item[p]);
  }
}

function dump_item_tree(item, indentation)
{
  if (!indentation)
  {
    indentation = "";
  }

  var str = indentation;
  if (item.objectName)
  {
    str += item.objectName + " ";
  }
  str += item;

  if (item.children && item.children.length > 0)
  {
    str += ", " + item.children.length + " children"
  }

  if (item.parent)
  {
    str += ", parent: ";
    if (parent.objectName)
    {
      str += item.parent.objectName + " ";
    }
    str += item.parent;
  }
  console.log(str);

  if (item.children)
  {
    var n = item.children.length;
    for (var i = 0; i < n; i++)
    {
      dump_item_tree(item.children[i], indentation + "  ");
    }
  }
}

function dump_path_to(item, indentation)
{
  if (!indentation)
  {
    indentation = "";
  }

  var str = indentation;
  if (item.objectName)
  {
    str += item.objectName + " ";
  }

  str += item;

  if (typeof(item.index) == "number")
  {
    str += ", index: " + item.index;
  }

  console.log(str);

  if (item.parent)
  {
    dump_path_to(item.parent, indentation + "  ");
  }
}
