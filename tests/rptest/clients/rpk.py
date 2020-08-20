import subprocess
import tempfile
import time


class RpkTool:
    """
    Wrapper around rpk.
    """
    def __init__(self, redpanda):
        self._redpanda = redpanda

    def create_topic(self, topic, partitions=1):
        cmd = ["topic", "create", topic]
        cmd += ["--partitions", str(partitions)]
        return self._run_api(cmd)

    def list_topics(self):
        cmd = ["topic", "list"]

        output = self._run_api(cmd)
        if "No topics found." in output:
            return []

        def topic_line(line):
            parts = line.split()
            assert len(parts) == 3
            return parts[0]

        lines = output.splitlines()
        for i, line in enumerate(lines):
            if line.split() == ["Name", "Partitions", "Replicas"]:
                return map(topic_line, lines[i + 1:])

        assert False, "Unexpected output format"

    def produce(self, topic, key, msg, headers=[], partition=None):
        cmd = [
            'produce', '--brokers',
            self._redpanda.brokers(), '--key', key, topic
        ]
        if headers:
            cmd += ['-H ' + h for h in headers]
        if partition:
            cmd += ['-p', str(partition)]
        return self._run_api(cmd, stdin=msg)

    def _run_api(self, cmd, stdin=None, timeout=30):
        cmd = [
            self._rpk_binary(), "api", "--brokers",
            self._redpanda.brokers(1)
        ] + cmd
        return self._execute(cmd, stdin=stdin, timeout=timeout)

    def _execute(self, cmd, stdin=None, timeout=30):
        self._redpanda.logger.debug("Executing command: %s", cmd)
        try:
            output = None
            f = subprocess.PIPE

            if stdin:
                f = tempfile.TemporaryFile()
                f.write(stdin)
                f.seek(0)

            # rpk logs everything on STDERR by default
            p = subprocess.Popen(cmd, stderr=subprocess.PIPE, stdin=f)
            start_time = time.time()

            ret = None
            while time.time() < start_time + timeout:
                ret = p.poll()
                if ret != None:
                    break
                time.sleep(0.5)

            if ret is None:
                p.terminate()

            if p.returncode:
                raise Exception('command %s returned %d' %
                                (' '.join(cmd), p.returncode))

            output = p.stderr.read()

            self._redpanda.logger.debug(output)
            return output
        except subprocess.CalledProcessError as e:
            self._redpanda.logger.debug("Error (%d) executing command: %s",
                                        e.returncode, e.output)

    def _rpk_binary(self):
        return self._redpanda.find_binary("rpk")
