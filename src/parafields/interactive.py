from parafields.field import load_schema, RandomField

import copy
import io
import numpy as np

try:
    # Check whether the extra requirements were installed
    import ipywidgets_jsonschema
    import ipywidgets
    import wrapt
    from IPython.display import display

    HAVE_JUPYYER_EXTRA = True
except ImportError:
    HAVE_JUPYYER_EXTRA = False


def return_proxy(creator, widgets):
    """A transparent proxy that can be returned from Jupyter UIs

    The created proxy object solves the general problem of needing to non-blockingly
    return from functions that display UI controls as a side effect. The returned object
    is updated whenever the widget state changes so that the return object would change.
    The proxy uses the :code:`wrapt` library to be as transparent as possible, allowing
    users to immediately work with the created object.
    :param creator:
        A callable that accepts no parameters and creates the object
        that should be returned. This is called whenever the widget
        state changes.
    :param widgets:
        A list of widgets whose changes should trigger a recreation of
        the proxy object.
    """

    class ObjectProxy(wrapt.ObjectProxy):
        def __copy__(self):
            return ObjectProxy(copy.copy(self.__wrapped__))

        def __deepcopy__(self, memo):
            return ObjectProxy(copy.deepcopy(self.__wrapped__, memo))

    # Create a new proxy object by calling the creator once
    proxy = ObjectProxy(creator())

    # Define a handler that updates the proxy
    def _update_proxy(_):
        proxy.__wrapped__ = creator()

    # Register handler that triggers proxy update
    for widget in widgets:
        widget.observe(
            _update_proxy, names=("value", "selected_index", "data"), type="change"
        )

    return proxy


def img_as_widget(img):
    """Create an image widget from a PIL.Image"""

    # Create and fill in-memory stream
    membuf = io.BytesIO()
    img.save(membuf, format="png")

    return ipywidgets.Image(value=membuf.getvalue(), format="png")


def interactive_generate_field(comm=None, partitioning=None, dtype=np.float64):
    """Interactively explore field generation in a Jupyter notebook"""

    # Return early if we do not have the extra
    if not HAVE_JUPYYER_EXTRA:
        print("Please re-run pip installation with 'parafields[jupyter]'")
        return

    # Create widgets for the configuration
    form = ipywidgets_jsonschema.Form(load_schema())

    # Output proxy object
    proxy = return_proxy(
        lambda: RandomField(
            form.data, comm=comm, partitioning=partitioning, dtype=dtype
        ),
        [form],
    )

    # Image widget for output
    imagebox = ipywidgets.Box()

    # Add a button that displays a realization of the field
    realize = ipywidgets.Button(
        description="Show realization", layout=ipywidgets.Layout(width="100%")
    )

    def _realize(_):
        imagebox.children = [img_as_widget(proxy._repr_png_())]

    realize.on_click(_realize)

    # Arrange the widgets into a grid layout
    app = ipywidgets.AppLayout(
        left_sidebar=form.widget,
        center=ipywidgets.VBox([realize, imagebox]),
        pane_widths=(1, 2, 0),
    )

    # Show the app as a side effect
    display(app)

    return proxy