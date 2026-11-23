import unittest

from local_worker import parse_review, protected_path


class LocalWorkerContracts(unittest.TestCase):
    def test_valid_review(self):
        result = parse_review('{"verdict":"pass","summary":"No blockers.","findings":[]}')
        self.assertEqual(result["verdict"], "pass")
        self.assertEqual(result["findings"], [])

    def test_review_with_leading_prose(self):
        result = parse_review(
            'Review result:\n{"verdict":"revise","summary":"Fix a bug.","findings":["bad bounds check"]}'
        )
        self.assertEqual(result["verdict"], "revise")

    def test_invalid_review_fails_closed(self):
        with self.assertRaises(RuntimeError):
            parse_review("Looks good to me.")

    def test_automation_paths_are_protected(self):
        for path in (
            ".github/workflows/local-ai.yml",
            "scripts/automation/local_worker.py",
            "docs/automation/strict-zero-cost.md",
            ".aiderignore",
        ):
            self.assertTrue(protected_path(path), path)

    def test_regular_native_source_is_editable(self):
        self.assertFalse(protected_path("native/pe-inspect/report.cpp"))
        self.assertFalse(protected_path("native/pe-inspect/report_test.cpp"))


if __name__ == "__main__":
    unittest.main()
