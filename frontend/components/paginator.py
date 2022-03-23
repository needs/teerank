"""
Paginator component.
"""

from flask import request, url_for


def _clamp(minval, maxval, val):
    """Clamp val between minval and maxval."""
    return max(minval, min(maxval, val))


PAGE_SIZE = 100
PAGE_SPREAD = 2

def info() -> tuple[int, int]:
    """
    Return the number of items to query as well as the offset.
    """

    try:
        page = int(request.args.get('page', 1))
    except ValueError:
        page = 1

    return PAGE_SIZE, (page - 1) * PAGE_SIZE


def init(count: int) -> dict:
    """
    Build paginator links.
    """

    links = []

    # URL settings for the page links.

    endpoint = request.endpoint
    args = request.args.copy()

    try:
        page = int(args.pop('page'))
    except KeyError:
        page = 1
    except ValueError:
        page = 1

    last_page = int((count - 1) / PAGE_SIZE) + 1
    page = _clamp(1, last_page, page)

    # Add the "Previous" button.

    links.append({
        'type': 'button',
        'name': 'Previous',
        'class': 'previous',
        'href': url_for(endpoint, page=page-1, **args) if page > 1 else None
    })

    # Add the first page.

    if page > PAGE_SPREAD + 1:
        links.append({
            'type': 'page',
            'page': 1,
            'href': url_for(endpoint, page=1, **args)
        })

    # Notice any missing pages between the fist page and the start of the page spread.

    if page > PAGE_SPREAD + 2:
        links.append({
            'type': '...'
        })

    # Start page spread.

    for i in range(page-PAGE_SPREAD, page):
        if i > 0:
            links.append({
                'type': 'page',
                'page': i,
                'href': url_for(endpoint, page=i, **args)
            })

    # Add the current page.

    links.append({
        'type': 'current',
        'page': page
    })

    # End page spread.

    for i in range(page + 1, page + PAGE_SPREAD + 1):
        if i <= last_page:
            links.append({
                'type': 'page',
                'page': i,
                'href': url_for(endpoint, page=i, **args)
            })

    # Notice any missing pages between the end of the page spread and the last page.

    if page < last_page - PAGE_SPREAD - 1:
        links.append({
            'type': '...'
        })

    # Add the last page.

    if page < last_page - PAGE_SPREAD:
        links.append({
            'type': 'page',
            'page': last_page,
            'href': url_for(endpoint, page=last_page, **args)
        })

    # Add the "Next" button.

    links.append({
        'type': 'button',
        'name': 'Next',
        'class': 'next',
        'href': url_for(endpoint, page=page+1, **args) if page < last_page else None
    })

    return {
        'first': PAGE_SIZE,
        'offset': (page - 1) * PAGE_SIZE,
        'links': links
    }
