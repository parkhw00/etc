
import logging

logger = logging.getLogger(__name__)

def sort_key(ch):
    if 'VCHANNEL' in ch:
        return float(ch['VCHANNEL'])
    return 0.

channels = list()

def load_from_file(filename):
    global channels

    logger.info (f'load from {filename}')
    cur = None
    with open(filename) as f:
        for l in f.read().splitlines():
            if len(l) < 2:
                True
            elif l[0] == '[' and l[-1] == ']':
                if cur:
                    channels.append(cur)

                cur = dict({('name', l[1:-1].strip())})
            else:
                prop = l.split(' = ')
                if len(prop) == 2 and cur:
                    key = prop[0].strip()
                    val = prop[1].strip()
                    cur[key] = val
                    logger.debug('ch[{0}] {1} = {2}'.format(cur['name'], key, val))

    if cur:
        channels.append(cur)

    channels.sort(key=sort_key)

def search(string):
    logger.debug(f'search "{string}"')
    ret=list()
    l=len(string)
    if l <= 0:
        return ret

    for c in channels:
        if 'VCHANNEL' in c and c['VCHANNEL'][:l] == string:
            logger.debug(f'append "{c}"')
            ret.append(c)
        elif 'name' in c and c['name'][:l] == string:
            logger.debug(f'append "{c}"')
            ret.append(c)

    return ret

def next(ch=None):
    if not ch:
        return channels[0]

    index = channels.index(ch) + 1
    if index >= len(channels):
        index = 0

    return channels[index]

def prev(ch=None):
    if not ch:
        return channels[-1]

    index = channels.index(ch) - 1
    return channels[index]

if __name__ == '__main__':
    import sys
    import argparse

    parser = argparse.ArgumentParser(description='channel database')
    parser.add_argument('--conf',
            help='channel list. output of dvbv5-scan. format of DVBV5')
    parser.add_argument('--loglevel',
            choices=['debug', 'info', 'warning', 'error', 'critical'],
            help='log level')
    parser.add_argument('--search',
            help='simple channel search that start with')

    args = parser.parse_args()

    if args.loglevel:
        logdict = dict([
            ('debug',    logging.DEBUG),
            ('info',     logging.INFO),
            ('warning',  logging.WARNING),
            ('error',    logging.ERROR),
            ('critical', logging.CRITICAL),
            ])
        logging.basicConfig(level=logdict[args.loglevel])
    logger.info(args)

    if args.conf:
        load_from_file(args.conf)

    if args.search:
        search = search(args.search)
        for c in search:
            print(c)
